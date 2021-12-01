# pokemon-exalted
Pokémon in Exalted

Este projeto possui uma adaptação de Pokémon para o sistema de RPG
Exalted e um protótipo feio, mas funcional em C do sistema de combate.

O programa de computador é em modo texto e uma pessoa pode jogar
contra o computador, duas pessoas podem jogar uma contra a outra ou o
computador pode jogar sozinho contra si mesmo. Por padrão é um jogo do
jogador contra a CPU. Se você passar "2" como argumento, vira um jogo
de dois jogadores humanos. E passando dois números como argumento, a
CPU joga contra ela mesma usando os Pokémons cujos números foram
passados como argumento.

O recuso de deixar a IA jogar contra ela mesma foi usada para gerar a
página seguinte que mostra quão bem cada Pokémon se sai lutando contra
outros e gerou uma tier list: [[https://thiagoharry.github.io/pokemon-exalted/]](https://thiagoharry.github.io/pokemon-exalted/).

Esta página descreve o que foi feito na adaptação.

#### Atributos e Habilidades

Um Pokémon tem como atributos "HP", "Ataque", "Ataque Especial",
"Defesa" e "Defesa Especial".

Em Exalted os Atributos são "Força", "Destreza", "Vigor", "Carisma",
"Manipulação", "Aparência", "Inteligência", "Raciocínio" e "Percepção".

A lógica utilizada para a tradução foi:

* O HP foi convertido para "Vitalidade", que não é um atributo. A
  conversão foi feita de modo que se hipoteticamente o Pokémon
  Shidinja fosse implementado, ele morreria sempre com um golpe e o
  segundo Pokémon com menos HP, o Diglett, pode sobreviver mesmo
  sofrendo um golpe que causa ferimento. Na prática isso significa que
  a cada 10HP, o Pokémon tem +1 de Vitalidade. Mas dano na Vitalidade
  em Exalted está associada a penalidades nos dados devido à dor. Da
  melhor forma possível busquei então diferenciar um Pokémon de 33HP
  de um de 37HP fazendo com que eles tivessem a mesma Vitalidade, mas
  os diferenciando fazendo com que o de 37HP tivesse um pouco menos de
  penalidade devido á dor. Desta forma, diferenças pequenas de HP
  apresentam efeitos perceptíveis.

* Força foi obtida à partir de Ataque. Vigor veio de
  Defesa. Inteligência veio de Ataque Especial. E Raciocínio veio de
  Defesa Especial. Fiz a conversão pegando os valores do Atributo do
  Pokémon e dividindo por 32. Isso faz com que em quase todos os
  Atributos, o Pokémon não-lendário com maior valor ficará com nível 5
  nele.

* Para o Carisma peguei o valor de "Base Friendship" e dividi por
  28. Isso faz com que o Pokémon com maior Carisma, a Chansey fique
  com o valor máximo de 5.

* Para a Manipulação, escolhi o quão difícil é treinar o Pokémon ao
  nível 100. De acordo com o estilo de ganho de XP dele. Um Pokémon
  que ganha XP rápido (Fast) tem 1 de Manipulação e o que tem um
  estilo de crescimento que torna mais difícil ser treinado
  (Fluctuating) tem 5 de Manipulação.

* Para a Percepção, escolhi de acordo com a probabilidade de encontrar
  o Pokémon se você o procurar na grama alta no lugar certo. Quanto
  mais fácil é achar o Pokémon, menor é a sua Percepção. Basicamente
  chance de captura de 3% ou menos é Percepção 5, captura de 6% ou
  menos é Percepção 4, captura de 12% ou menso é Percepção 3, captura
  de 25% ou menos é Percepção 2, captura de 50% ou menos é Percepção
  nível 1. Acima disso a Percepção é 0. Pokémons que não podem ser
  capturados na grama tem Percepção 5. Eu levo em conta apenas jogos
  da geração na qual o Pokémon apareceu pela primeira vez, não nas
  gerações posteriores.

* A Aparência foi obtida à partir da Exp. Base do Pokémon, o quanto
  ele tende a dar de XP ao ser derrotado. Foi o que pensei que
  simularia uma primeira impressão.

* Força de Vontade não é bem um Atributo, mas eu a obtive à partir da
  taxa de captura do Pokémon, o quão provável é ele escapar de uma
  Poké-bola. A fórmula é (255-Catch Rate)/25.5. Enquanto tem Força de
  Vontade, os Pokémons sempre irão usá-la automaticamente em todo
  ataque para garantir sucesso automático ou aumentar a defesa em 1
  ponto.

* Habilidades: Assumo que todo Pokémon tem o mesmo nível em toda
  Habilidade (Briga, Esquiva, Furtividade, ...) O valor é igual ao
  quanto de Effort Values (EVs) o Pokémon concede ao ser derrotado. E
  varia então entre 1 e 3.

* O tamanho dos Pokémons foi levado em conta para aplicar as vantagens
  "Tiny Body" e "Legendary Size".

#### Tipos de Ataques

Todos os Pokémons podem fazer ataques decisivos e não-decisivos
(withering attacks). Os não-decisivos aparecem como ataques genéricos
a serem escolhidos. Todo Pokémon tem como ataque não-decisivo dois
tiopos: os físicos e especiais.

Os ataques físicos seguem exatamente as regras de ataques
não-decisivos (withering) de Exalted. São considerados sempre ataques
de curta distância (close distance).

Ataques não-decisivos contam também com precisão (accuracy), absorção
(soak) e dano (damage). Tais valores foram obtidos caso o Pokémon
tenha algo que aumente sua velocidade, ataque ou defesa. Por exemplo,
se ele sabe o golpe Agility que dá +2 de Velocidade, eu assumo uma
precisão de 2 para ele. Trato de maneira separada o dano e a absorção
de ataques físicos e especiais. E tais valores não são cumulativos. Se
um Pokémon sabe Agility (+2 velocidade) e Dragon Dance (+1 Velocidade,
+1 Ataque), isso dá a ele 2 de Precisão e 1 de Dano.

Os ataques especiais seguem quase as mesmas regras dos ataques
não-decisivos de Exalted. Apenas troca-se a Força pela Inteligência e
Vigor pelo Raciocínio para computar o dano.

Pokémons que tem golpes como Psyshock e Psystrike tem um terceiro tipo
de ataque, que aqui chamei de ataques mentais. Eles trocam a Força
pela Inteligência, mas não o Vigor pelo Raciocínio.

Ao escolher um ataque não-decisivo, não é necessário escolher qual
tipo de ataque será dado: o Pokémon irá escolher sozinho o que for
mais vantajoso.

A maior parte dos movimentos dos Pokémons que causam dano foi
convertida para um ataque decisivo padrão. Qual ataque o Pokémon irá
usar depende da quantidade de Iniciativa que ele tem. Quanto maior a
Iniciativa, ele irá usar golpes mais poderosos. Mas o que realmente
causa o dano é a quantidade de Iniciativa acumulada, o nome do ataque
usado geralmente é só cosmético.

Entretanto, os ataques decisivos tem sempre um Tipo e um Alcance. O
Tipo é um dos tipos de Pokémon (Fogo, Água, Fada, Metal, Dragão, ...)
e as forças e fraquezas de cada Pokémon foram implementadas. Se ele é
fraco contra água e toma um ataque decisivo de água, sofrerá +1 de
dano. Se for duplamente fraco contra água, sofrerá +2 de
dano. Resistência é implementada como -1 de dano ou -2 se for
resistência dupla.

O Alcance dos ataques decisivos depende apenas se o golpe faz contato
com o alvo ou não. No primeiro caso é um alcance de "close distance",
no segundo de "short distance".

Pokémons sem ataques não podem dar ataques decisivos. É o caso do
Metapod, Kakuna, Abra e Ditto caso ele não se transforme.

Todos os Pokémons também tem a opção de esperar e segurar seu ataque
para atacar ao mesmo tempo que o inimigo, desde que tenham mais
Iniciativa que ele. Isso segue as regras de segurar o ataque para
provocar "Clash Attacks".

#### Ataques com Regras Especiais

Se um Pokémon tem algum ataque de prioridade (Quick Attack, Extreme
Speed, Ice Shard, ...), ele sempre ganha 1 dado a mais para rodar sua
Iniciativa inicial. Em outras palavras, ele ganha a vantagem "Reflexos
Rápidos" (Exalted, pág. 161).

O ataque "U-Turn" foi implementado como uma ação múltipla ("flurry")
de dar um ataque decisivo e se afastar ("disengage").

Os golpes "Protect" e "Detect" foram implementados como sendo uma ação
de Defesa Total em combate.

Os golpes "Smokescreen" e "Sand Attack" foram implementados como a
vantagem "Ink Sacs/Smokescreen" (Lunares, pág. 120). Pokémons que não
precisam da visão para se locomover são imunes ao ataque. São todos
aqueles que conhecem o golpe "Supersonic" que tratei como a vantagem
"Echolocalization" (Lunares, pág 119).

Alguns ataques carregam um turno e atacam no próximo: Solar Beam,
Skull Bash e Sky Attack. Tais ataques foram implementados seguindo as
regras de mirar (Aim) um ataque. Já Pokémons com o golpe Lock-On e
Laser Focus podem usar a ação Mirar (aim) e escolher o ataque que será
beneficiado pela mira n o outro turno, ao invés de ter que dar sempre
o mesmo ataque decisivo.

Alguns ataques que tornam o Pokémon invulnerável no turno em que está
sendo carregado (Dig, Dive) foram implementados como ativar Stealth em
um turno e atacar no próximo seguindo as regras de ataques
inesperados (-2 de defesa).

Fazer dormir (Sleep Powder, Hypnosis, Sing, Yawn) foram implementados
com as regras da magia "Mists of Eventide".

Golpes como Light Screen foram implementados seguindo as regras de
"Take Cover".

Golpes como Stealth Rock, Spikes e Toxic Spikes apenas tornam o
terreno difícil para o oponente.

Os golpes Bind, Fire Spin, Sand Tomb, Whirlpool e Wrap foram tratados
seguindo as regras de "Grapple". Entretanto, mesmo após agarrar o
alvo, só é permitido jogar o alvo contra o chão se o Pokémon tiver os
golpes Slam, Circle Throw, Dragon Tail, Whirlwind ou Seismic Toss. Se
ele tiver um destes golpes sem um de Grapple, o Pokémon os usa dando
uma ação de Grapple e executando o arremesso logo em seguida. Pokémons
que tenham múltiplos corpos (Magneton, Dugtrio) nunca podem ser alvo
de Grapple. Pokémons bípedes, se arremessados, ficam caídos no chão
depois do ataque e precisam se levantar. Pokémons com 4 ou 6 patas
podem ficar caídos, mas só se forem arremessados por oponentes bem
maiores. Os demais Pokémons não sofrem qualquer penalidade nem ficam
caídos ao serem jogados no chão.

O golpe Fake Out só pode ser usado no primeiro turno, se o Pokémon
tiver mais Iniciativa que o alvo e se for bem-sucedido em teste de
Furtividade (Stealth) feito automaticamente antes da partida
começar. Ele é um ataque não-decisivo que trata a defesa do alvo como
zero. Ou seja, segue as regras de Emboscada (Ambush).

Há golpes de interação social. O golpe Roar é tratado usando regras de
Intimidação. Mas só pode ser usado se o oponente tiver mais machucado
e com menso Iniciativa. Ou se o oponente tiver sido assustado antes
com Noble Roar, Scary Face ou Leer. Tais golpes são tratados como
emoções inspiradas por meio de Performance. Estar assustado pode
limitar o que o alvo pode fazer nos turnos próximos. Ataques e
movimentos não-condizentes com estar assustado ficam desativados
dependendo do quão bem-sucedido foi o Pokémon que o usou.

Golpes como Taunt e Swagger, ao invés de inspirar medo, inspiram
raiva. O alvo, se afetado, por uns turnos só pode usar ataques.

Golpes Growl, Fake Tears, Play Nice, Tickle e Tail Whip fazem o alvo
ficar com pena de machucar você. Entretanto, a duração do efeito é
reduzida se você o atacar.

Golpes como Baby Doll Eyes, Captivate, Sweet Kiss e Charm inspiram o
alvo a se apaixonar. Não funciona em Pokémons sem gênero. Nos demais,
há 50\% de chance deles falharem automaticamente pelo seu Pokémon não
ser do interesse amoroso do oponente. Alvos apaixonados não podem
atacar ou fazer ações hostis por alguns turnos. O efeito termina mais
rápido se você atacar o alvo, mas ele não vai desaparecer
completamente.

O golpe Lovely Kiss, aqui, ao invés de fazer dormir é tratado como uma
tentativa de encerrar a luta e convencer o alvo a se render por meio
da sedução. Ele só funciona se antes cocê fizer o alvo se apaixonar.

Veneno é tratado segundo a vantagem "Venomous" (Exalted, pág. 167). Os
golpes Poison Powder, Poison Gas e Toxic são implementados seguindo a
regra de cuspir o veneno. Golpes que podem envenenar se acertar, se
usados como ataques decisivos, irão envenenar desde que haja veneno
disponível.

Paralisia de golpes como Thunder Wave e Stun Spore são tratadas assim
como veneno, com a diferença de que se o alvo tiver sua Iniciativa
quebrada, ao invés de sofrer dano na Vitalidade, apenas não poderão
usar ações de movimento (regras de venenos de paralisia, livro dos
Lunares). O mesmo vale para a paralisia de ataques decisivos
elétricos. Eu uso o mesmo contador de veneno para contar quantas vezes
um Pokémon pode paralisar outro.

Já o Glare é implementado segundo as regras do olhar de basilisco
(Hundred Devil Night Parade). Basicamente é um Grapple a distância em
que o alvo não pode fazer nada enquanto ele dura.

Por fim, paralisia de ataques como Body Slam é tratada com as regras
de "Bone-Crunching Bite" para ataques que causam penalidades
adicionais (crippling penalties). Exalted, pág. 566.

Por fim, ataques que fazem o alvo pegar fogo, como Will-o-Wisp são
tratados também usando o mesmo contador de doses de veneno. Mas eles
não envenenam. Ao invés disso, usando as regras do livro dos
Dragon-Born, o alvo em chamas toma 1 dado de dano na Vitalidade ao fim
de cada rodada. Apagar o fogo é uma ação miscelânea. Para Pokémons de
água é só usar ação miscelânea. Os demais apagam se jogando no
chão. Pokémons bípedes após apagarem o fogo ficarão caídos no chão,
pois assumo que apagaram rolando no chão.

O golpe Transform, do Ditto e Mew, foi implementado seguindo as regras
do "Rival Nature Subsumption" do demônio Metody (Hundred Devil Night
Parade).

Teleport é tratado como Disengage.