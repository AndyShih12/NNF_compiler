/****************************************************************************************
 * The Sentential Decision Diagram Package
 * sdd version 2.0, January 8, 2018
 * http://reasoning.cs.ucla.edu/sdd
 ****************************************************************************************/

#include "sdd.h"

//manager/manager.c
SddManager* sdd_manager_create(SddLiteral var_count, int auto_gc_and_minimize);

//manager/interface.c
int sdd_node_is_true(SddNode* node);
int sdd_node_is_false(SddNode* node);
int sdd_node_is_literal(SddNode* node);
int sdd_node_is_decision(SddNode* node);
SddLiteral sdd_node_literal(SddNode* node);
SddNode* sdd_manager_literal(const SddLiteral literal, const SddManager* manager);

//basic/shadows.c
// SddNode* shadow_node(NodeShadow* shadow);
// ElmShadow* shadow_elements(NodeShadow* shadow);
// int shadow_is_terminal(NodeShadow* shadow);
// SddShadows* shadows_new(SddSize root_count, SddNode** root_nodes, SddManager* manager);
// void shadows_traverse(void (*fn)(NodeShadow*,SddShadows*), SddShadows* shadows);
// void shadows_free(SddShadows* shadows);

// //sdds/apply.c
// SddNode* apply(SddNode* node1, SddNode* node2, BoolOp op, SddManager* manager, int limited);

// //local declarations
// static void initialize_for_shadows(SddNode* node, int* exists_map, SddManager* manager);
// static SddNode* quantify_shadow(NodeShadow* shadow, int* exists_map, SddManager* manager);
// static void ref_nodes_of_terminal_shadows(SddShadows* shadows);
// static void deref_nodes_of_terminal_shadows(SddShadows* shadows);


/****************************************************************************************
 * Compiles a NNF bottom-up
 ****************************************************************************************/

long hash(long a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

LongHashMap* hash_init(long size) {
  LongHashMap* hashmap = malloc(size * sizeof(LongHashMap));
  hashmap->size = size;
  hashmap->lists = calloc(size, sizeof(HashElement*));
  hashmap->list_sizes = calloc(size, sizeof(long*));
  hashmap->list_capacities = calloc(size, sizeof(long*));
  return hashmap;
}

long hash_get(long x, LongHashMap* hashmap) {
  long bucket = hash(x) % hashmap->size;

  //walk down linked list and check for hit
  for (int i = 0; i < hashmap->list_sizes[bucket]; i++) {
    long k = hashmap->lists[bucket][i].key;
    if (k == x) {
      return hashmap->lists[bucket][i].value;
    }
  }

  return 0;
}

long hash_set(long k, long v, LongHashMap* hashmap) {
  long z = hash_get(k, hashmap);
  if ((void*)(z) != NULL) {
    if (z != v) return 1;
    return 0;
  }

  long bucket = hash(k) % hashmap->size;

  //walk down linked list and insert
  if (hashmap->list_sizes[bucket] == 0) {
    hashmap->list_capacities[bucket] = 10;
    hashmap->lists[bucket] = malloc(hashmap->list_capacities[bucket] * sizeof(HashElement));
  }
  else if (hashmap->list_sizes[bucket] == hashmap->list_capacities[bucket]) {
    hashmap->list_capacities[bucket] = 2*hashmap->list_sizes[bucket];
    
    HashElement* tmp = malloc(hashmap->list_sizes[bucket] * sizeof(HashElement));
    for (int i = 0; i < hashmap->list_sizes[bucket]; i++) {
      tmp[i].key = hashmap->lists[bucket][i].key;
      tmp[i].value = hashmap->lists[bucket][i].value;
    }
    free(hashmap->lists[bucket]);

    hashmap->lists[bucket] = malloc(hashmap->list_capacities[bucket] * sizeof(HashElement));
    for (int i = 0; i < hashmap->list_sizes[bucket]; i++) {
      hashmap->lists[bucket][i].key = tmp[i].key;
      hashmap->lists[bucket][i].value = tmp[i].value;
    }
    free(tmp);
  }

  long sz = hashmap->list_sizes[bucket];
  hashmap->lists[bucket][sz].key = k;
  hashmap->lists[bucket][sz].value = v;
  hashmap->list_sizes[bucket] += 1;

  return 0;
}

void hash_free(LongHashMap* hashmap) {
  for (int i = 0; i < hashmap->size; i++) {
    free(hashmap->lists[i]);
  }
  free(hashmap->lists);
  free(hashmap->list_sizes);
  free(hashmap->list_capacities);
  free(hashmap);
}


///////////////////////////////////////

NnfNode* init_nnf_node(NnfNodeType type, NnfNodeSize size) {
  NnfNode* nnf = malloc(sizeof(NnfNode));
  nnf->type = type;
  nnf->size = size;
  if (size) { nnf->children = malloc(size * sizeof(struct NnfNode*)); }
  nnf->node = NULL;
  nnf->ref_count = 0;
  return nnf;
}

NnfNode* init_nnf_lit(NnfLiteral lit) {
  NnfNode* nnf = init_nnf_node(NNF_LIT, 0);
  nnf->literal = lit;
  return nnf;
}

//assumes only one root
NnfNode* read_nnf_from_file(const char* filename, int* var_count_ptr) {
  char* buffer = read_file(filename);
  char* filtered = filter_comments(buffer);


  const char* whitespace = " \t\n\v\f\r";
  char* token = strtok(filtered, whitespace);
  assert(strcmp(token, "nnf") == 0);

  int node_count = int_strtok();
  /*int edge_count = */int_strtok();
  int var_count = int_strtok();  

  NnfNode** node_ptrs = malloc(node_count * sizeof(struct NnfNode*));

  int node_id = 0;
  token = strtok(NULL, whitespace);
  while (token != NULL) {
    if (strcmp(token, "L") == 0) {
      int literal = int_strtok();
      node_ptrs[node_id] = init_nnf_lit(literal);
    } else if (strcmp(token, "A") == 0 || strcmp(token, "O") == 0) {
      int num_children = 0;
      if ((strcmp(token, "A") == 0)) {
        num_children = int_strtok();
        node_ptrs[node_id] = init_nnf_node(NNF_AND, num_children);
      }
      else {
        int_strtok(); // ignore an int
        num_children = int_strtok();
        node_ptrs[node_id] = init_nnf_node(NNF_OR, num_children);
      }

      for (int i = 0; i < num_children; i++) {
        int child_id = int_strtok();
        node_ptrs[node_id]->children[i] = node_ptrs[child_id];
        node_ptrs[child_id]->ref_count += 1;
      }
    }

    node_id += 1;
    token = strtok(NULL, whitespace);
  }

  NnfNode* root = node_ptrs[node_count-1];
  root->ref_count += 1;
  free(node_ptrs);

  *var_count_ptr = var_count;
  return root;
}

NnfNode* nnf_replace_helper(NnfNode* nnf, NnfLiteral lit, NnfNode* replace, LongHashMap* hashmap) {
  if (nnf->type == NNF_LIT) {
    if (nnf->literal == lit) {
      replace->ref_count += 1;
      return replace;
    }
    return nnf;
  }

  NnfNode* node = (NnfNode*)(hash_get((long)(nnf), hashmap));
  if (node) {
    return node;
  }
  hash_set((long)(nnf), (long)(nnf), hashmap);


  for (int i = 0; i < nnf->size; i++) {
    nnf->children[i] = nnf_replace_helper(nnf->children[i], lit, replace, hashmap);
  }
  return nnf;
}

NnfNode* nnf_replace(NnfNode* nnf, NnfLiteral lit, NnfNode* replace) {
  int hash_size = 160001;
  LongHashMap* hashmap = hash_init(hash_size);
  nnf = nnf_replace_helper(nnf, lit, replace, hashmap);
  hash_free(hashmap);
  return nnf;
}

SddNode* nnf_to_sdd(NnfNode* nnf, SddManager* manager) {
  if (nnf->node) {
    return nnf->node;
  }
  if (nnf->type == NNF_LIT) {
    nnf->node = sdd_manager_literal(nnf->literal, manager);
    return nnf->node;
  }

  BoolOp op = 0;
  if (nnf->type == NNF_AND) {
    nnf->node = sdd_manager_true(manager);
    op = CONJOIN;
  }
  else if (nnf->type == NNF_OR) {
    nnf->node = sdd_manager_false(manager);
    op = DISJOIN;
  }

  for (int i = 0; i < nnf->size; i++) {
    SddNode* beta = nnf_to_sdd(nnf->children[i], manager);
    SddNode* gamma = sdd_apply(nnf->node, beta, op, manager);
    sdd_deref(beta, manager);
    sdd_deref(nnf->node, manager);
    nnf->node = gamma;
    sdd_ref(nnf->node, manager);
  }
  sdd_deref(nnf->node, manager);

  for (int i = 0; i < nnf->ref_count; i++) {
    sdd_ref(nnf->node, manager);
  }

  return nnf->node;
}

NnfNode* sdd_to_nnf_helper(SddNode* sdd, SddManager* manager, LongHashMap* hashmap, int pad) {
  NnfNode* nnf = NULL;

  if (sdd_node_is_decision(sdd)) {
    SddNodeSize size = sdd->size;
    nnf = init_nnf_node(NNF_OR, size);

    for(int i = 0; i < size; i++) {
      SddElement* e = ELEMENTS_OF(sdd) + i;
      NnfNode* prime = (NnfNode*)(hash_get((long)(e->prime), hashmap));
      NnfNode* sub = (NnfNode*)(hash_get((long)(e->sub), hashmap));

      if (prime == NULL) {
        prime = sdd_to_nnf_helper(e->prime, manager, hashmap, pad);
        hash_set((long)(e->prime), (long)(prime), hashmap);
      }
      if (sub == NULL) {
        sub = sdd_to_nnf_helper(e->sub, manager, hashmap, pad);
        hash_set((long)(e->sub), (long)(sub), hashmap);
      }

      nnf->children[i] = init_nnf_node(NNF_AND, 2);
      nnf->children[i]->children[0] = prime;
      nnf->children[i]->children[1] = sub;

      nnf->children[i]->ref_count += 1;
      nnf->children[i]->children[0]->ref_count += 1;
      nnf->children[i]->children[1]->ref_count += 1;
    }
  } else if (sdd_node_is_literal(sdd)) {
    int signed_pad = sdd_node_literal(sdd) > 0 ? pad : -1 * pad;
    SddLiteral padded_lit = sdd_node_literal(sdd) + signed_pad;
    nnf = init_nnf_lit(padded_lit);
  } else if (sdd_node_is_true(sdd)) {
    nnf = init_nnf_node(NNF_AND, 0);
  } else if (sdd_node_is_false(sdd)) {
    nnf = init_nnf_node(NNF_OR, 0);
  }

  return nnf;
}

NnfNode* sdd_to_nnf_pad_literals(SddNode* sdd, SddManager* manager, int pad) {
  int hash_size = 160001;
  LongHashMap* hashmap = hash_init(hash_size);
  NnfNode* nnf = sdd_to_nnf_helper(sdd, manager, hashmap, pad);
  hash_free(hashmap);

  nnf->ref_count += 1;
  return nnf;
}

NnfNode* sdd_to_nnf(SddNode* sdd, SddManager* manager) {
  return sdd_to_nnf_pad_literals(sdd, manager, 0);
}

// watch out for double free
void free_nnf(NnfNode* nnf) {
  nnf->ref_count -= 1;
  if (!nnf->ref_count) {
    if (nnf->size) {
      for (int i = 0; i < nnf->size; i++) {
        free_nnf(nnf->children[i]);
      }
      free(nnf->children);
    }
    free(nnf);
  }
}
