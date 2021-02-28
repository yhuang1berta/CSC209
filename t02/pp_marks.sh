#!/bin/bash

#declare integer variables assignment_total and tutorial_total
declare -i assignment_total=0
declare -i assignment_score_total=0
declare -i tutorial_total=0
declare -i tutorial_score_total=0
#looping through directories to get the corresponding mark
for d in a{1..4}; do
	if [ -d "$d" ]; then
		if [ -f "$d/feedback.txt" ]; then
		grade=`cat $d/feedback.txt | tr -d [:blank:] | tail -n 1` #delete all horizontal spaces in feedback.txt and save it in variable grade.

		IFS=' / ' read -ra grArray <<<"$grade" #split grade by '/' and save them in an array variable.
		score=${grArray[0]}
		max=${grArray[1]}
		assignment_total=$assignment_total+$max
		assignment_score_total=$assignment_score_total+$score
		echo "$d: $score / $max"
		else
			echo "$d: - / -"
		fi
	else
		echo "$d: - / -"
	fi
done

for d in t{01..11}; do
	if [ -d "$d/" ]; then
		if [ -f "$d/feedback.txt" ]; then
			grade=`cat $d/feedback.txt | tr -d [:blank:] | tail -n 1`

			IFS=' / ' read -ra grArray <<<"$grade"

			score=${grArray[0]}
			max=${grArray[1]}
			tutorial_total=$tutorial_total+$max
			tutorial_score_total=$tutorial_score_total+$score
			echo "$d: $score / $max"
		else
			echo "$d: - / -"
		fi
	else
		echo "$d: - / -"
	fi
done		

echo "Assignment Total: $assignment_score_total / $assignment_total"
echo "Tutorial Total: $tutorial_score_total / $tutorial_total"
