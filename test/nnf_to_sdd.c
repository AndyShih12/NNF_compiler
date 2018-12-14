#include "sddapi.h"

int main() {
	char* nnf_filename = "./c432.isc.cnf.nnf";

	int var_count = 0;
	NnfNode* nnf = read_nnf_from_file(nnf_filename, &var_count);
	SddManager* manager = sdd_manager_create(var_count, 1);
	printf("Var count: %d\n", var_count);

	SddNode* alpha = nnf_to_sdd(nnf, manager);
	free_nnf(nnf);

	printf("done first free\n");

	int pad = 10;
	var_count += pad;
	nnf = sdd_to_nnf_pad_literals(alpha, manager, pad);
	manager = sdd_manager_create(var_count, 1);
	alpha = nnf_to_sdd(nnf, manager);
	free_nnf(nnf);

	printf("%ld\n", sdd_manager_size(manager));
	printf("%llu\n", sdd_global_model_count(alpha,manager));

	return 0;
}