#!/usr/bin/env python

#Usage "./compress.py image.h"
#Takes the .h file exported from gimp and implements run-length encoding to compress the image.
#Basically, the script reads through the char array four bytes at a time (four bytes represents the 
#color of the pixel) and will store the number of iterations associated with the pixel color if it repeats
#along a row of the image. Colors that are not repeated are just stored without an iteration count.
#Iteration counts are delimited with a " " so that when the extract function parses through the compressed #char array, the extract function knows how to discern between a color and an iteration count.


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
	if line == "static char *header_data =" + "\n":
		f.write("static char *header_data_compressed_" + output_filename + "=" + "\n")
		begin_compress = True
	elif begin_compress == True:
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
pixel_count = 0
while j < len(line_buffer) - 4:
	slash_count = 0
	# Ensures that we have four characters, excludes the escape characters
#	print(len(line_buffer[j:j+4+slash_count]) - line_buffer[j:j+4+slash_count].count('\\'))
#	print("This is the data:" + line_buffer[j:j+4+slash_count])
	while len(line_buffer[j:j+4+slash_count]) - line_buffer[j:j+4+slash_count].count('\\') + line_buffer[j:j+4+slash_count].count("\\\\") < 4: 
		slash_count = line_buffer[j:j+4+slash_count].count('\\') - line_buffer[j:j+4+slash_count].count("\\\\")
#		print("This is the data:" + line_buffer[j:j+4+slash_count])
#		print(slash_count)
		

	if line_buffer[j:j+4+slash_count] == last_color:  #j+4 not included in the comparison
		color_count += 1
	else: # Save the last color to line_output and reset the color count if new color
		if color_count > 1:
			line_output += last_color 
			if len(hex(color_count)) == 6: # Four hex digits
				color_count_hex = " \\x" + str(hex(color_count)[2:4]) + "\\x" + str(hex(color_count)[4:6])
			elif len(hex(color_count)) == 5: # Three hex digits
				color_count_hex = " \\x" + str(hex(color_count)[2:3]) + "\\x" + str(hex(color_count)[3:5])
			elif len(hex(color_count)) == 4: # Two hex digits
				color_count_hex = " \\x0\\x" + str(hex(color_count)[2:4])
			elif len(hex(color_count)) == 3: # One hex digit
				color_count_hex = " \\x0\\x" + str(hex(color_count)[2:3])
			else: # Not a valid state
				color_count_hex = "ERROR"
			line_output += color_count_hex + " "
			pixel_count += color_count
		else:
			line_output += last_color 
			pixel_count += 1
		color_count = 1 
		last_color = line_buffer[j:j+4+slash_count] 
#		print(line_buffer[j:j+4+slash_count])
	j += 4 + slash_count #Increment by pixel
# Last iteration of loop needs to written
if color_count > 1:
	line_output += last_color 
	if len(hex(color_count)) == 6: # Four hex digits
		color_count_hex = " \\x" + str(hex(color_count)[2:4]) + "\\x" + str(hex(color_count)[4:6])
	elif len(hex(color_count)) == 5: # Three hex digits
		color_count_hex = " \\x" + str(hex(color_count)[2:3]) + "\\x" + str(hex(color_count)[3:5])
	elif len(hex(color_count)) == 4: # Two hex digits
		color_count_hex = " \\x0\\x" + str(hex(color_count)[2:4])
	elif len(hex(color_count)) == 3: # One hex digit
		color_count_hex = " \\x0\\x" + str(hex(color_count)[2:3])
	else: # Not a valid state
		color_count_hex = "ERROR"
	line_output += color_count_hex + " "
	pixel_count += color_count
else:
	line_output += last_color 
	pixel_count += 1	
f.write("\"" + line_output + "\";")
print(pixel_count)
			


f.close() 
