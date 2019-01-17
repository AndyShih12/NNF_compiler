#include "sddapi.h"

int main(int argc, char **argv) {

  char* filename = "cnn.nnf";
  //char* dot_filename = "arthur/test.dot";
  int var_count = -1;
  NnfNode* nnf = read_nnf_from_file(filename, &var_count);
  printf("var count: %d\n", var_count);
  
  printf("total nnf node count: %lld\n", global_nnf_count());


  SddManager* manager = sdd_manager_create(var_count,1);
  SddNode* sdd = nnf_to_sdd(nnf, manager);
  printf("sdd node count: %zu\n", sdd_count(sdd));
  printf("sdd size: %zu\n", sdd_size(sdd));
  printf("model count: %lld\n", sdd_model_count(sdd,manager));
  printf("global model count: %lld\n", sdd_global_model_count(sdd,manager));

  free_nnf(nnf);
  printf("total nnf node count: %lld\n", global_nnf_count());

  //sdd_save_as_dot(dot_filename,sdd);
  return 0;
}
