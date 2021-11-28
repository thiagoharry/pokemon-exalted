#!/bin/bash

# This scripts generates a github markdown table with battle results

names = [ "Bulbasaur" "Ivysaur" "Venusaur" "Charmander" "Charmeleon" "Charizard"
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
	  "Weepinbell" "Victreebel" "Tentacool" "Tentacruel" "Geodude" ""
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
	  "Dragonair" "Dragonite" "Mewtwo" "Mew"]

img_url = ["2/21/001MS8.png" "3/38/002MS8.png" "b/ba/003MS8.png"
	   "d/dc/004MS8.png" "b/b8/005MS8.png" "a/a4/006MS8.png"
	   "d/dc/007MS8.png" "a/af/008MS8.png" "f/f0/009MS8.png"
	   "6/61/010MS8.png" "f/f9/011MS8.png" "f/fe/012MS8.png"
	   "b/bc/013MSPE.png" "7/70/014MSPE.png" "a/a2/015MSPE.png"
	   "5/56/016MSPE.png" "c/cc/017MSPE.png" "9/93/018MSPE.png"
	   "7/7a/019MSPE.png" "7/77/020MSPE.png" "a/af/021MSPE.png"
	   "7/76/022MSPE.png" "a/a7/023MSPE.png" "2/2c/024MSPE.png"
	   "6/66/025MS8.png" "b/b5/026MS8.png" "a/ac/027MS8.png"
	   "b/b6/028MS8.png" "8/89/029MS8.png" "9/9e/030MS8.png"
	   "8/8d/031MS8.png" "4/4a/032MS8.png" "7/75/033MS8.png"
	   "8/8a/034MS8.png" "c/cf/035MS8.png" "9/97/036MS8.png"
	   "6/6d/037MS8.png" "6/61/038MS8.png" "f/f3/039MS8.png"
	   "c/c6/040MS8.png" "0/0c/041MS8.png" "c/ce/042MS8.png"
	   "6/68/043MS8.png" "f/f5/044MS8.png" "a/a5/045MS8.png"
	   "6/6a/046MSPE.png" "4/40/047MSPE.png" "3/30/048MSPE.png"
	   "a/a7/049MSPE.png" "f/f1/050MS8.png" "2/2a/051MS8.png"
	   "e/e4/052MS8.png" "f/f7/053MS8.png" "a/a0/054MS8.png"
	   "7/7f/055MS8.png" "a/af/056MSPE.png" "a/aa/057MSPE.png"
	   "4/45/058MS8.png" "c/cc/059MS8.png" "c/c7/060MS8.png"
	   "b/b1/061MS8.png" "e/e4/062MS8.png" "4/4e/063MS8.png"
	   "d/dc/064MS8.png" "4/41/065MS8.png" "4/4c/066MS8.png"
	   "1/13/067MS8.png" "8/87/068MS8.png" "4/41/069MSPE.png"
	   "2/2e/070MSPE.png" "4/4b/071MSPE.png" "6/6f/072MS8.png"
	   "6/6c/073MS8.png" "0/04/074MSPE.png" "0/00/075MSPE.png"
	   "a/a9/076MSPE.png" "3/30/077MS8.png" "2/2b/078MS8.png"
	   "3/31/079MS8.png" "c/cb/080MS8.png" "e/ea/081MS8.png"
	   "d/da/082MS8.png" "2/23/083MS8.png" "5/58/084MSPE.png"
	   "3/3b/085MSPE.png" "3/37/086MSPE.png" "a/a7/087MSPE.png"
	   "6/63/088MSPE.png" "d/d1/089MSPE.png" "8/8f/090MS8.png"
	   "c/ce/091MS8.png" "8/8e/092MS8.png" "e/e8/093MS8.png"
	   "5/58/094MS8.png" "5/5e/095MS8.png" "1/12/096MSPE.png"
	   "6/64/097MSPE.png" "b/be/098MS8.png" "f/fd/099MS8.png"
	   "3/31/100MSPE.png" "3/31/101MSPE.png" "0/07/102MS8.png"
	   "f/fb/103MS8.png" "8/88/104MS8.png" "7/72/105MS8.png"
	   "5/5d/106MS8.png" "8/81/107MS8.png" "3/32/108MS8.png"
	   "c/c7/109MS8.png" "5/58/110MS8.png" "e/e1/111MS8.png"
	   "0/04/112MS8.png" "f/f1/113MS8.png" "b/b3/114MS8.png"
	   "7/74/115MS8.png" "6/65/116MS8.png" "a/ae/117MS8.png"
	   "7/71/118MS8.png" "5/59/119MS8.png" "0/04/120MS8.png"
	   "2/2b/121MS8.png" "e/e6/122MS8.png" "e/e6/123MS8.png"
	   "9/90/124MS8.png" "f/f0/125MS8.png" "3/3c/126MS8.png"
	   "b/b8/127MS8.png" "b/be/128MS8.png" "d/d0/129MS8.png"
	   "8/8e/130MS8.png" "c/c3/131MS8.png" "9/92/132MS8.png"
	   "1/19/133MS8.png" "c/ca/134MS8.png" "8/8c/135MS8.png"
	   "e/e5/136MS8.png" "b/bc/137MS8.png" "c/c2/138MS8.png"
	   "8/80/139MS8.png" "a/a8/140MS8.png" "4/4c/141MS8.png"
	   "9/90/142MS8.png" "5/5a/143MS8.png" "a/a0/144MS8.png"
	   "4/4b/145MS8.png" "f/f6/146MS8.png" "9/9d/147MS8.png"
	   "b/b7/148MS8.png" "5/55/149MS8.png" "3/3e/150MS8.png"
	   "b/b0/151MS8.png"]

echo -n "| "
for (( i=1; i <= 151; i++ )); do
    echo -n "![${names[$i]}](https://archives.bulbagrden.net/media/upload/${url[$i]}) |"
done
for (( i=1; i <= 151; i++ )); do
    for (( j=0; j <= 151; j++ )); do
	if [ ${j} -eq 0 ]; then
	    echo -n "![${names[$i]}](https://archives.bulbagrden.net/media/upload/${url[$i]}) |"
	else
	    echo -n " |"
	fi
	if [ ${j} -eq 151 ]; then
	    echo
	fi
    done
done
    
