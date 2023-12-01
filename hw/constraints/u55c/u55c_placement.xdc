# # pblk_hbm0
# create_pblock pblk_hbm0
# resize_pblock pblk_hbm0 -add SLICE_X0Y0:SLICE_X28Y59
# add_cells_to_pblock pblk_hbm0 [get_cells [list inst_int_hbm/path_0]]
# add_cells_to_pblock pblk_hbm0 [get_cells [list inst_int_hbm/path_1]]
# add_cells_to_pblock pblk_hbm0 [get_cells [list inst_int_hbm/path_2]]
# add_cells_to_pblock pblk_hbm0 [get_cells [list inst_int_hbm/path_3]]

# # pblk_hbm1
# create_pblock pblk_hbm1
# resize_pblock pblk_hbm1 -add SLICE_X29Y0:SLICE_X57Y59
# add_cells_to_pblock pblk_hbm1 [get_cells [list inst_int_hbm/path_4]]
# add_cells_to_pblock pblk_hbm1 [get_cells [list inst_int_hbm/path_5]]
# add_cells_to_pblock pblk_hbm1 [get_cells [list inst_int_hbm/path_6]]
# add_cells_to_pblock pblk_hbm1 [get_cells [list inst_int_hbm/path_7]]

# # pblk_hbm2
# create_pblock pblk_hbm2
# resize_pblock pblk_hbm2 -add SLICE_X58Y0:SLICE_X86Y59
# add_cells_to_pblock pblk_hbm2 [get_cells [list inst_int_hbm/path_8]]
# add_cells_to_pblock pblk_hbm2 [get_cells [list inst_int_hbm/path_9]]
# add_cells_to_pblock pblk_hbm2 [get_cells [list inst_int_hbm/path_10]]
# add_cells_to_pblock pblk_hbm2 [get_cells [list inst_int_hbm/path_11]]

# # pblk_hbm3
# create_pblock pblk_hbm3
# resize_pblock pblk_hbm3 -add SLICE_X87Y0:SLICE_X115Y59
# add_cells_to_pblock pblk_hbm3 [get_cells [list inst_int_hbm/path_12]]
# add_cells_to_pblock pblk_hbm3 [get_cells [list inst_int_hbm/path_13]]
# add_cells_to_pblock pblk_hbm3 [get_cells [list inst_int_hbm/path_14]]
# add_cells_to_pblock pblk_hbm3 [get_cells [list inst_int_hbm/path_15]]

# pblk_net_module_0
create_pblock pblk_net_module_0
resize_pblock pblk_net_module_0 -add CLOCKREGION_X0Y6:CLOCKREGION_X0Y7
add_cells_to_pblock pblk_net_module_0 [get_cells [list inst_network_top_0/inst_network_module]]

# pblk_net_early_ccross_0
create_pblock pblk_net_early_ccross_0
resize_pblock pblk_net_early_ccross_0 -add CLOCKREGION_X0Y4:CLOCKREGION_X0Y5
add_cells_to_pblock pblk_net_early_ccross_0 [get_cells [list inst_network_top_0/inst_early_ccross]]

# pblk_net_stack_0
create_pblock pblk_net_stack_0
resize_pblock pblk_net_stack_0 -add CLOCKREGION_X1Y4:CLOCKREGION_X3Y7
add_cells_to_pblock pblk_net_stack_0 [get_cells [list inst_network_top_0/inst_network_stack]]

# # app: dedup
# create_pblock slr1
# resize_pblock slr1 -add CLOCKREGION_X0Y4:CLOCKREGION_X7Y7
# add_cells_to_pblock slr1 [get_cells [list inst_dynamic/inst_dedup/dedupCore]]


