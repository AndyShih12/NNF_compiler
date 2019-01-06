#include "sddapi.h"

int main() {
  const int N = 32;
	int base_var_count = 75;
  int total_var_count = base_var_count + N;

  char file[N][100];
  int i = 0;
  for (i = 0; i < N; i++) {
    sprintf(file[i], "../circuits/ionosphere_majority_tree_%d.sdd", i);
  }

  SddManager* manager = sdd_manager_create(total_var_count, 1);
  NnfNode* nnfs[2][N];

  for (i = 0; i < N; i++) {
  	SddNode* alpha = sdd_read(file[i], manager);
    nnfs[0][i] = sdd_to_nnf(sdd_negate(alpha, manager), manager);
    nnfs[1][i] = sdd_to_nnf(alpha, manager);
  }

  SddNode* majsdd = sdd_read("../circuits/ionosphere_majority.sdd", manager);
  NnfNode* maj = sdd_to_nnf(majsdd, manager);

  printf("here2\n");

  for (i = 0; i < N; i++) {
  	NnfLiteral lit = base_var_count + (i+1);

  	maj = nnf_replace(maj, -1*lit, nnfs[0][i]);
  	maj = nnf_replace(maj, lit, nnfs[1][i]);
  }

	printf("here3\n");
	fflush(stdout);

	//manager = sdd_manager_create(base_var_count, 1);
	SddNode* sdd = nnf_to_sdd(maj, manager);

	printf("model count: %lld\n", sdd_model_count(sdd,manager));

  return 0;

	// char* nnf_filename = "./c432.isc.cnf.nnf";

	// int var_count = 0;
	// NnfNode* nnf = read_nnf_from_file(nnf_filename, &var_count);
	// SddManager* manager = sdd_manager_create(var_count, 1);
	// printf("Var count: %d\n", var_count);

	// SddNode* alpha = nnf_to_sdd(nnf, manager);
	// free_nnf(nnf);

	// printf("done first free\n");

	// int pad = 10;
	// var_count += pad;
	// nnf = sdd_to_nnf_pad_literals(alpha, manager, pad);
	// manager = sdd_manager_create(var_count, 1);
	// alpha = nnf_to_sdd(nnf, manager);
	// free_nnf(nnf);

	// printf("%ld\n", sdd_manager_size(manager));
	// printf("%llu\n", sdd_global_model_count(alpha,manager));

	// return 0;
}