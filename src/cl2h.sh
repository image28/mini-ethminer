INPUT="$1"
OUTPUT="$2"

echo -en "static char const kernelSource[] = {\n" > $OUTPUT
hexdump -ve '1/1 "0x%02x, "' $INPUT | rev | cut -c3- | rev >> $OUTPUT
echo -en "};" >> $OUTPUT
