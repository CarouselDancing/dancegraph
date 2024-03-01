import re

def add_public_to_variable( line ):
    stripped = line.strip(' \t{}')
    # Leave comments as they are
    if (not stripped) or (stripped[0] == '/'):
        return line
    i = 0
    for i,c in enumerate(line):
        if c != ' ' and c != '\t':
            break
    return line[:i] + 'public ' + line[i:]

code = open('../net/config.h','rt').read()

code = code.replace('struct', '[Serializable] public class')
code = code.replace('std::','')
code = code.replace('unordered_map','Dictionary')

lines = code.split('\n')
lines_out = ['using System;', 'using System.Collections.Generic;']


# in_class assumes the outer {} encloses a namespace
in_class = 0
for line in lines:
    if '}' in line:
        in_class -= 1
    # Ignore preprocessor
    if line.startswith('#'):
        continue
    # Ignore functions
    if '(' in line:
        continue
    if 'vector<' in line:
        line = line.replace('vector<','').replace('>','[]')
    if in_class > 1:
        line = add_public_to_variable(line)
    if '{' in line:
        in_class += 1
    lines_out.append(line)
    
code = "\n".join(lines_out) + "\n"
open('NetConfig.cs','wt').write(code)
        
    