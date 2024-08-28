# Image compressor generated with https://lcd-image-converter.riuson.com/ with RLE.
# Author : C.Braillard
# Date : 02.08.2024

import re
import string

# Compress an hardcoded image in.h with Run-Length Encoding (RLE)
imgName = input('Path of the image: ')

# imgName = './img/ors_neon.h'
imgNameSpl = imgName.split('.h')
newImageName = imgNameSpl[0] + '_comp.h'


fileHead = ''

file = open(imgName, 'r') #heaer file to compress
data = file.read()        # read file

dataSplit = data.split('= {\n')           # Separate start of file and rest
fileHead = dataSplit[0] + ' = { \n\t'   # header file start

# Separate head to have split start to '[' and split from ']' to {
head = re.split(r'(\[\d+\])',fileHead)


dataSplit2 = dataSplit[1].split('}')    # Separate image datas and end of file

dataUtils = dataSplit2[0]               # Util datas

printable = set(string.printable)-set(' \t\n')       # define regular expression
dataUtils = ''.join(filter(lambda x: x in printable, dataUtils))    #Remove non printable character of dataUtils

dataArray = dataUtils.split(',')        # Array of util datas

counter = 0                             # Counter of same symbol
currentSymbol = dataArray[0]                      # Symbol to count
newDataArray = []                       # New Array of util datas
for hexa in dataArray:                   # Read array of util datas (dataArray)
    if currentSymbol != hexa:            # Check if next symbol is same as previous or not. If not do:
        newDataArray.append(currentSymbol.replace(' ', ''))        #If yes fill newDataArray with this pattern (nbTime, symbol) example (0x000A, 0x0000) means 10 times black color
        newDataArray.append(hex(counter).upper())
        currentSymbol = hexa             # Change current symbol
        counter = 1
    else: 
        counter += 1

newDataArray.append(currentSymbol.replace(' ', ''))        #If yes fill newDataArray with this pattern (nbTime, symbol) example (0x000A, 0x0000) means 10 times black color
newDataArray.append(hex(counter).upper())

n = len(newDataArray)

newdata = ', '.join(map(str, newDataArray)) 

defineORS_N = '#define ' + imgNameSpl[0].upper() + '_SIZE ' + str(n)

nFile = open(newImageName, 'w')
nFile.write(head[0] + '[' + str(n) + ']' + head[2] + '\n' + newdata + '\n};\n' + defineORS_N)

# print(dataArray, '\n')
    