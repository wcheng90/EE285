#!/usr/bin/env python

#Currently you can only have up to 2^13-1 iterations of the same color for compression to be properly stoed
#This is roughly 4000

import sys
f = open(sys.argv[1],"r")
lines = f.readlines()
f.close()

i = 0
output_filename = ""
while sys.argv[1][i] != ".":
	output_filename += sys.argv[1][i]
	i += 1
	
f = open(output_filename + "_compressed.h","w+")
begin_compress = False
line_buffer = ""
for line in lines:
	i = 0
	if line != "static char *header_data =" + "\n" and begin_compress == False:
		f.write(line)
	elif line == "static char *header_data =" + "\n":
		f.write("static char *header_data_compressed =" + "\n")
		begin_compress = True
	else:
		if line != "\t\"\";\n": #ignores last line
			while line[i] != "\"":
				i += 1
			i += 1 # Moves pointer past the double quotes
			#print len(line)
			while i < len(line) - 2: # Don't want to rewrite new line or
				line_buffer += line[i]
				i += 1
#print(line_buffer)
j = 0 
last_color = ""
line_output = ""
color_count = 1
color_count_hex = ""
while j < len(line_buffer):

	if line_buffer[j:j+4] == last_color:  #j+4 not included in the comparison
		color_count += 1
	else: # Save the last color to line_output and reset the color count if new color
		if color_count > 1:
			line_output += last_color 
			if len(hex(color_count)) < 5:
				if len(hex(color_count)) == 4:
					color_count_hex = " 0" + str(hex(color_count)[2:])
				else: #Length is three here
					color_count_hex = " 00" + str(hex(color_count)[2:])
			else:
				color_count_hex = " " + str(hex(color_count)[2:])
			line_output += color_count_hex
		else:
			line_output += last_color 
		color_count = 1
		last_color = line_buffer[j:j+4]
	j += 4 #Increment by pixel
# Last iteration of loop needs to written
if color_count > 1:
	line_output += last_color 
	if len(hex(color_count)) < 5:
		if len(hex(color_count)) == 4:
			color_count_hex = " 0" + str(hex(color_count)[2:])
		else: #Length is three here
			color_count_hex = " 00" + str(hex(color_count)[2:])
	else:
		color_count_hex = " " + str(hex(color_count)[2:])
	line_output += color_count_hex
else:
	line_output += last_color 	
f.write("\"" + line_output + "\"")

			


f.close()
