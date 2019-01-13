#include "sddapi.h"

int main(int argc, char **argv) {

  int CONJOIN_SDD, digit;
  digit = atoi(argv[1]);
  CONJOIN_SDD = atoi(argv[2]);
  char* basename = argv[3];

  char sdd_outname[100];
  char vtree_outname[100];
  sprintf(sdd_outname, "%s.sdd", basename);
  sprintf(vtree_outname, "%s.vtree", basename);

  const int N = 9;
  const int F = 2; // filters
  const int base_var_count = 8 * 8;
  const int total_var_count = base_var_count + N * F;

  char sddfile[F * N][100];
  //char vtreefile[F * N][100];
  int i = 0, j = 0;
  for (i = 0; i < F; i++) {
    for (j = 0; j < N; j++) {
      sprintf(sddfile[i*N + j], "/space/andyshih2/4andy/cnn/conv1_9_%d_%d.sdd", i, j);
      //sprintf(vtreefile[i*N + j], "/space/andyshih2/4andy/cnn/conv1_9_%d_%d.vtree", i, j);
    }
  }

  char csdd[100], cvtree[100];
  sprintf(csdd, "/space/andyshih2/4andy/cnn/%d_wd1_%d.sdd", F*N, digit);
  sprintf(cvtree, "/space/andyshih2/4andy/cnn/%d_wd1_%d.vtree", F*N, digit);

  Vtree* vtree = sdd_vtree_read(cvtree);
  SddManager* manager = sdd_manager_new(vtree);
  sdd_manager_auto_gc_and_minimize_on(manager);
  SddNode* main_sdd = sdd_read(csdd, manager);

  printf("%d\n", sdd_manager_is_auto_gc_and_minimize_on(manager));

/////////////////////////////////////////////////

  if (CONJOIN_SDD) {
    sdd_ref(main_sdd, manager);

    for (i = 0; i < F*N; i++) {
      int lit = base_var_count + (i+1);

      SddNode* alpha = sdd_read(sddfile[i], manager);
      sdd_ref(alpha, manager);
      
      SddNode* beta1 = sdd_disjoin(alpha, sdd_manager_literal(-1 * lit, manager), manager);
      sdd_ref(beta1, manager);
      SddNode* beta2 = sdd_disjoin(sdd_negate(alpha, manager), sdd_manager_literal(lit, manager), manager);
      sdd_ref(beta2, manager);

      sdd_deref(alpha, manager);
      alpha = sdd_conjoin(beta1, beta2, manager);
      sdd_ref(alpha, manager);
      sdd_deref(beta1, manager);
      sdd_deref(beta2, manager);

      sdd_deref(main_sdd, manager);
      main_sdd = sdd_conjoin(main_sdd, alpha, manager);
      sdd_ref(main_sdd, manager);
      sdd_deref(alpha, manager);

      sdd_manager_minimize_limited(manager);

      printf("read sdd #%d\n", i);
      printf("current node count: %zd\n", sdd_count(main_sdd));
      fflush(stdout);
    }

    printf("node count: %zd\n", sdd_count(main_sdd));
    printf("model count: %lld\n", sdd_global_model_count(main_sdd,manager));
    printf("Existentially forgetting...\n");
  
    int exists_map[total_var_count + 1];
    for (i = 0; i <= total_var_count; i++) {
      exists_map[i] = (i > base_var_count) ? 1 : 0;
    }
    main_sdd = sdd_exists_multiple(exists_map, main_sdd, manager);

    printf("node count: %zd\n", sdd_count(main_sdd));
    printf("model count: %lld\n", sdd_global_model_count(main_sdd,manager));

    sdd_save(sdd_outname, main_sdd);
    sdd_vtree_save(vtree_outname, sdd_manager_vtree(manager));

    return 0;
  }



  NnfNode* nnfs[2][F*N];

  for (i = 0; i < F*N; i++) {
    //Vtree* vtree = sdd_vtree_read(vtreefile[i]);
    SddManager* manager_tmp = sdd_manager_create(base_var_count ,1);
    SddNode* alpha = sdd_read(sddfile[i], manager_tmp);
    printf("read sdd #%d\n", i);
    fflush(stdout);

    nnfs[0][i] = sdd_to_nnf(sdd_negate(alpha, manager_tmp), manager_tmp);
    nnfs[1][i] = sdd_to_nnf(alpha, manager_tmp);

    printf("read nnf #%d\n", i);
    fflush(stdout);

    sdd_manager_free(manager_tmp);
  }

  NnfNode* main_nnf = sdd_to_nnf(main_sdd, manager);

  printf("here2\n");
  fflush(stdout);

  for (i = 0; i < F*N; i++) {
    NnfLiteral lit = base_var_count + (i+1);

    main_nnf = nnf_replace(main_nnf, -1*lit, nnfs[0][i]);
    main_nnf = nnf_replace(main_nnf, lit, nnfs[1][i]);
  }

  printf("here3\n");
  fflush(stdout);

  //manager = sdd_manager_create(base_var_count, 1);
  SddNode* sdd = nnf_to_sdd(main_nnf, manager);

  printf("node count: %zd\n", sdd_count(sdd));
  printf("model count: %lld\n", sdd_model_count(sdd,manager));
  printf("global model count: %lld\n", sdd_global_model_count(sdd,manager));


  sdd_save(sdd_outname, sdd);
  sdd_vtree_save(vtree_outname, sdd_manager_vtree(manager));

  return 0;
}
