#include "sddapi.h"

int main(int argc, char **argv) {

  int GRID, folder, CONJOIN_SDD;
  GRID = atoi(argv[1]);
  CONJOIN_SDD = atoi(argv[2]);
  folder = atoi(argv[3]);
  char* basename = argv[4];

  char sdd_outname[100];
  char vtree_outname[100];
  sprintf(sdd_outname, "%s.sdd", basename);
  sprintf(vtree_outname, "%s.vtree", basename);

  const int N = 5;
  const int base_var_count = GRID * GRID;
  const int total_var_count = base_var_count + N;

  char sddfile[N][100];
  char vtreefile[N][100];
  int i = 0;
  for (i = 0; i < N; i++) {
    sprintf(sddfile[i], "/space/andyshih2/4andy/%dx%d/%d/5_h1_%d.sdd", GRID, GRID, folder, i);
    sprintf(vtreefile[i], "/space/andyshih2/4andy/%dx%d/%d/5_h1_%d.vtree", GRID, GRID, folder, i);
  }

  char c[100];
  sprintf(c, "/space/andyshih2/4andy/%dx%d/%d/5_out_0.sdd", GRID, GRID, folder);
  SddManager* manager = sdd_manager_create(total_var_count, 1);
  SddNode* main_sdd = sdd_read(c, manager);

/////////////////////////////////////////////////

  if (CONJOIN_SDD) {
    NnfNode* nnf = sdd_to_nnf_pad_literals(main_sdd, manager, base_var_count);
    main_sdd = nnf_to_sdd(nnf, manager);
    sdd_ref(main_sdd, manager);

    for (i = 0; i < N; i++) {
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

      printf("read sdd #%d\n", i);
      fflush(stdout);
    }

    int exists_map[total_var_count + 1];
    for (i = 0; i <= total_var_count; i++) {
      exists_map[i] = (i > base_var_count) ? 1 : 0;
    }
    main_sdd = sdd_exists_multiple(exists_map, main_sdd, manager);
    
    printf("model count: %lld\n", sdd_model_count(main_sdd,manager));
    return 0;
  }



  NnfNode* nnfs[2][N];

  for (i = 0; i < N; i++) {
    Vtree* vtree = sdd_vtree_read(vtreefile[i]);
    SddManager* manager_tmp = sdd_manager_new(vtree);
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

  const int padding = 123456;

  for (i = 0; i < N; i++) {
    NnfLiteral lit = base_var_count + (i+1) + padding;
    NnfNode* node0 = init_nnf_lit(-1 * lit);
    NnfNode* node1 = init_nnf_lit(lit);

    main_nnf = nnf_replace(main_nnf, -1*(i+1), node0);
    main_nnf = nnf_replace(main_nnf, (i+1), node1);
  }

  for (i = 0; i < N; i++) {
    NnfLiteral lit = base_var_count + (i+1) + padding;

    main_nnf = nnf_replace(main_nnf, -1*lit, nnfs[0][i]);
    main_nnf = nnf_replace(main_nnf, lit, nnfs[1][i]);
  }

  printf("here3\n");
  fflush(stdout);

  //manager = sdd_manager_create(base_var_count, 1);
  SddNode* sdd = nnf_to_sdd(main_nnf, manager);

  printf("model count: %lld\n", sdd_model_count(sdd,manager));


  sdd_save(sdd_outname, sdd);
  sdd_vtree_save(vtree_outname, sdd_manager_vtree(manager));

  return 0;
}