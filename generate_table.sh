#!/bin/bash

# This scripts generates a github markdown table with battle results

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
echo "<table><tr>"
for (( i=1; i <= 151; i++ )); do
    #echo -n "![${names[$i]}](https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png) |"
    echo "<td><img alt=\"${names[$i]}\" src=\"https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png\"/></td>"
done
#echo
#echo -n "| "
echo "</tr>"
#for (( i=0; i <= 151; i++ )); do
#    echo -n "--- |"
#done
#echo
for (( i=1; i <= 151; i++ )); do
    echo "<tr>"
    for (( j=0; j <= 151; j++ )); do
	if [ ${j} -eq 0 ]; then
	    #echo -n " ![${names[$i]}](https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png) |"
	    echo "<td><img alt=\"${names[$i]}\" src=\"https://github.com/thiagoharry/pokemon-exalted/blob/main/images/${i}.png\"/></td>" 
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
	    vitoria=$(grep -c "${names[$i]}" result.log)
	    empate=$(grep -c EMPATE result.log)
	    derrota=$(grep -c "${names[$j]}" result.log)
	    vermelho=$((2 * ${derrota} + ${empate}))
	    verde=$((2*${derrota} + ${empate}))
	    total=$((2*${vitoria} + 2*${derrota} + ${empate}))
	    vermelho=$((255*${vermelho}/${total}))
	    verde=$((255*${verde}/${total}))
	    vermelho=$(printf "%.2X\n" ${vermelho})
	    verde=$(printf "%.2X\n" ${verde})
	    #echo -n " ![${vitoria} vitórias, ${empate} empates, ${derrota} derrotas](https://via.placeholder.com/15/${vermelho}${verde}00/000000?text=+) |"
	    echo "<td><img alt=\"${vitoria} vitórias, ${empate} empates, ${derrota} derrotas\" src=\"https://via.placeholder.com/15/${vermelho}${verde}00/000000?text=+\"/></td>"
	fi
    done
    echo "</tr>"
done
echo "</table>"
    
