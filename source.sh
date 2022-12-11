#!/bin/bash
stack=()
push() { stack+=("$@"); }
peek() { printf %s\\n "${stack[-1]}"; }
pop() { peek; unset 'stack[-1]'; }
data < $(sudo ./encoder/encoder -q)

while [ true ]
do
	sleep 1.5
	push $data
	peek encoder_line
	$encoder_line | xargs -I {} sudo ./display_text/lcd "{}"
    echo $encoder_line
done
