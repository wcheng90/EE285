#!/usr/bin/env python

# Usage "./addimages.py main.c"
# This script will modify main.c to add the various compressed images into the build by adding the proper
# include statements and proper arrays call outs in the method that extracts the images. 

import fnmatch
import os
import sys
f = open(sys.argv[1],"r")
lines = f.readlines()
f.close()
	
f = open(sys.argv[1],"w+")
begin_compress = False
line_buffer = ""
image_num = 0 
for line in lines:
	if line == "//Images Here" + "\n":
		f.write(line)
		for headerfiles in os.listdir("./scripts/pokemon"):
			if fnmatch.fnmatch(headerfiles, '*_compressed.h'):
				include_line = "#include \"scripts/pokemon/" + headerfiles + "\"" + "\n"
				f.write(include_line)
				print(include_line)
	elif line == "\t" + "if (image_num == 0){" + "\n":
		for headerfiles in os.listdir("./scripts/pokemon"):
			if fnmatch.fnmatch(headerfiles, '*_compressed.h'):
				if image_num == 0:
					f.write("\t" + "if (image_num == " + str(image_num) + "){" + "\n")	
				else:
					f.write("\t" + "}" + "\n")
					f.write("\t" + "else if (image_num == " + str(image_num) + "){" + "\n")
				f.write("\t" +"\t" + "pointer_mux = header_data_compressed_" + headerfiles[:-13] + ";\n")
				
				image_num += 1
	else:
		f.write(line)

f.close()
