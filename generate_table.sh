#!/bin/bash

# This scripts generates a github markdown table with battle results

declare -a tier
tier[0]=0
maximum=0

names=(" " "Bulbasaur" "Ivysaur" "Venusaur" "Charmander" "Charmeleon" "Charizard"
       "Squirtle" "Wartortle" "Blastoise" "Caterpie" "Metapod" "Butterfree"
       "Weedle" "Kakuna" "Beedrill"  "Pidgey" "Pidgeotto" "Pidgeot" "Rattata"
       "Raticate" "Spearow" "Fearow" "Ekans" "Arbok" "Pikachu"  "Raichu"
       "Sandshrew" "Sandslash" "Nidoran♀"  "Nidorina" "Nidoqueen" "Nidoran♂"
       "Nidorino" "Nidoking" "Clefairy" "Clefable" "Vulpix" "Ninetales"
       "Jigglypuff"  "Wigglytuff" "Zubat" "Golbat" "Oddish" "Gloom"
       "Vileplume" "Paras" "Parasect" "Venonat" "Venomoth" "Digllet"
       "Dugtrio" "Meowth" "Persian" "Psyduck" "Golduck" "Mankey" "Primeape"
       "Growlithe" "Arcanine" "Poliwag" "Poliwhirl" "Poliwrath" "Abra"
       "Kadabra" "Alakazam" "Machop" "Machoke" "Machamp" "Bellsprout"
       "Weepinbell" "Victreebel" "Tentacool" "Tentacruel" "Geodude"
       "Graveler" "Golem" "Ponyta" "Rapidash" "Slowpoke" "Slowbro"
       "Magnemite" "Magneton" "Farfetch'd" "Doduo" "Dodrio" "Seel" "Dewgong"
       "Grimer" "Muk" "Shellder" "Cloyster" "Gastly" "Haunter" "Gengar"
       "Onix" "Drowzee" "Hypno" "Krabby" "Kingler" "Voltorb"  "Electrode"
       "Exeggcute" "Exeggutor" "Cubone" "Marowak" "Hitmonlee" "Hitmonchan"
       "Lickitung" "Koffing" "Weezing" "Rhyhorn" "Rhydon" "Chansey" "Tangela" 
       "Kangaskhan" "Horsea" "Seadra" "Goldeen" "Seaking" "Staryu" "Starmie"
       "Mr. Mime" "Scyther" "Jynx" "Electabuzz" "Magmar" "Pinsir" "Tauros"
       "Magikarp" "Gyarados" "Lapras" "Ditto" "Eevee" "Vaporeon" "Jolteon"
       "Flareon" "Porygon" "Omanyte" "Omastar" "Kabuto" "Kabutops"
       "Aerodactyl" "Snorlax" "Articuno"  "Zapdos" "Moltres" "Dratini"
       "Dragonair" "Dragonite" "Mewtwo" "Mew")

#echo -n "| "

echo "<style>"
echo  "img{width: 68px; height: 68px; margin: -11px 0 0 -11px;}"
echo "* {box-sizing: border-box; }"
echo "table { table-layout: fixed;  border-collapse: collapse; width: 100%; max-width: 100px; }"      
echo "td { width: 46px; height: 46px; overflow: hidden; white-space: nowrap; }"
echo "</style>"

echo "<table><tr>"
echo "<td></td>"
for (( i=1; i <= 151; i++ )); do
    #echo -n "![${names[$i]}](https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png) |"
    echo "<td><img alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/></td>"
done
#echo
#echo -n "| "
echo "</tr>"
#for (( i=0; i <= 151; i++ )); do
#    echo -n "--- |"
#done
#echo
for (( i=1; i <= 151; i++ )); do
    sum=0
    echo "<tr>"
    for (( j=0; j <= 151; j++ )); do
	if [ ${j} -eq 0 ]; then
	    #echo -n " ![${names[$i]}](https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png) |"
	    echo "<td><img alt=\"${names[$i]}\" width=\"46px\" height=\"46px\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/></td>" 
	elif [ ${j} -eq ${i} ]; then
	    #echo " | "
	    echo "<td></td>"
	else
	    # Fazer o teste aqui
	    echo "" > result.log
	    for (( t=0; t < 100; t++ )); do
		./pokemon-exalted ${i} ${j} > /dev/null 2>> result.log
	    done
	    >&2 echo "${names[${i}]} x ${names[${j}]}"
	    vitoria=$(grep -c "${names[$i]} ganhou" result.log)
	    sum=$((${sum}+${vitoria}))
	    empate=$(grep -c EMPATE result.log)
	    derrota=$(grep -c "${names[$j]} ganhou" result.log)
	    vermelho=$((2 * ${derrota} + ${empate}))
	    verde=$((2*${vitoria} + ${empate}))
	    total=$((${verde} + ${vermelho}))
	    vermelho=$((255*${vermelho}/${total}))
	    verde=$((255*${verde}/${total}))
	    vermelho=$(printf "%.2X\n" ${vermelho})
	    verde=$(printf "%.2X\n" ${verde})
	    #echo -n " ![${vitoria} vitórias, ${empate} empates, ${derrota} derrotas](https://via.placeholder.com/15/${vermelho}${verde}00/000000?text=+) |"
	    echo "<td><img title=\"${names[${i}]} x ${names[${j}]}: ${vitoria} vitórias, ${empate} empates, ${derrota} derrotas\" src=\"https://via.placeholder.com/15/${vermelho}${verde}00/000000?text=+\"/></td>"
	fi
    done
    echo "</tr>"
    tier[${i}]=${sum}
    if [ ${sum} -gt ${maximum} ]; then
	maximum=${sum}
    fi
done
echo "</table>"

echo "<table><tr>"
echo "<td>S</td><td>"
for (( i=1; i <= 151; i++ )); do
    if [ ${tier[$i]} -ge $((4*${maximum}/5)) ]; then
	echo "<img caption=\"${tier[$i]} vitórias\" alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/>"
    fi
done
echo "</td></tr><tr><td>A</td><td>"
for (( i=1; i <= 151; i++ )); do
    if [ ${tier[$i]} -ge $((3*${maximum}/5)) -a ${tier[$i]} -lt $((4*${maximum}/5)) ]; then
	echo "<img caption=\"${tier[$i]} vitórias\" alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/>"
    fi
done
echo "</td></tr><tr><td>B</td><td>"
for (( i=1; i <= 151; i++ )); do
    if [ ${tier[$i]} -ge $((2*${maximum}/5)) -a ${tier[$i]} -lt $((3*${maximum}/5)) ]; then
	echo "<img caption=\"${tier[$i]} vitórias\" alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/>"
    fi
done
echo "</td></tr><tr><td>C</td><td>"
for (( i=1; i <= 151; i++ )); do
    if [ ${tier[$i]} -ge $((${maximum}/5)) -a ${tier[$i]} -lt $((2*${maximum}/5)) ]; then
	echo "<img caption=\"${tier[$i]} vitórias\" alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/>"
    fi
done
echo "</td></tr><tr><td>D</td><td>"
for (( i=1; i <= 151; i++ )); do
    if [ ${tier[$i]}  -lt $((${maximum}/5)) ]; then
	echo "<img caption=\"${tier[$i]} vitórias\" alt=\"${names[$i]}\" src=\"https://raw.githubusercontent.com/thiagoharry/pokemon-exalted/main/images/${i}.png\"/>"
    fi
done
echo "</td></tr></table>"

