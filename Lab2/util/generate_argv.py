import sys
import random

file_name = sys.argv[1]
argv = sys.argv[2:]
argc = len(argv)
assert(argc >= 0)

cookie = ["726277617368657265_;)", "i2c_has_2_wires", "tayato_writes_great_software"]
f = open(file_name, "w")
f.truncate()
split_arg = repr(argv).replace('\'', '\"')[1:-1]
autogen_header = "/**************************THIS FILE WAS AUTOGENERATED**************************/\n"
line = '#include <stddef.h>\nstatic char const *user_argv[] = {{\"{}\", {}, NULL}};\nstatic const int user_argc = {};\n'.format(random.choice(cookie), split_arg, str(argc + 1))
autogen_footer = "/*******************************************************************************/"
f.write(autogen_header + line + autogen_footer)
f.close()