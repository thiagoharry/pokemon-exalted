#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#include <sys/time.h>
#include <sys/random.h>
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#endif
#include "random.h"

static bool test;
int test_pokemon1, test_pokemon2;

// Configuration macros
#define AUTOMATIC_WILLPOWER

// Utility macros
#define MAX(a, b) ((a>=b)?(a):(b))
#define GETCHAR() {if(!test){						\
    struct timeval t0, t1;						\
    gettimeofday(&t0, NULL);						\
    do{ getchar();							\
      gettimeofday(&t1, NULL);						\
    }while(1000000*(t1.tv_sec - t0.tv_sec) +				\
	   t1.tv_usec - t0.tv_usec < 500000);				\
    }}
#define GET_BONUS(p) (p -> penalty[p -> damage] + \
		      ((p -> poisoned || p -> sleepy || \
			p -> paralyzed)?(-3):(0)))
#define GET_RESOLVE(p) ((p -> wits + p -> essence +	\
			 GET_BONUS(p)) / 2.0)
#define GET_RESOLVE_INT(p) ((RAND()%2)?					\
			    ((p -> wits + p -> essence +		\
			      GET_BONUS(p)) / 2):			\
			    ((p -> wits + p -> essence +		\
			      GET_BONUS(p) + 1) / 2))
#define GET_THREATEN_DICE(p) (p -> manipulation + p -> essence +	\
			      GET_BONUS(p))
#define GET_MANIPULATION_DICE(p) (p -> manipulation + p -> essence +	\
			      GET_BONUS(p))
#define GET_CHARISMA_DICE(p) (p -> manipulation + p -> essence +	\
			      GET_BONUS(p))
#define GET_MAGIC_DICE(p) (p -> inteligence + p -> essence +	\
			   GET_BONUS(p))
#define GET_DEFENSE(p) ((p -> dexterity + p -> essence +	\
			 (p -> crippling_penalty) +		\
			 GET_BONUS(p)) / 2.0)
#define GET_BONUS_PARRY(p1, p2)						\
  (p1 -> onslaugh_penalty +						\
   (p1 -> crippling_penalty) +						\
   ((p1 -> prone)?(-1):(0)) +						\
   ((p1 -> blinded)?(-1.5):(0)) +					\
   ((p1 -> covered > 0)?						\
    ((p1 -> covered > 1)?(2):(1)):(0)) +				\
   ((p1 -> grappled)?(-2):(0)) +					\
   ((p1 -> control_turns > 0)?(-2):(0)) +				\
   ((p1 -> full_defense)?(2):(0)))
#define GET_BONUS_EVASION(p1, p2)				\
  (((p1 -> size == TINY && p2 -> size > TINY)?(2.0):(0.0)) +	\
   p1 -> onslaugh_penalty + ((p1 -> blinded)?(-1.5):(0)) +	\
   (p1 -> crippling_penalty) +					\
   ((p1 -> covered > 0)?((p1 -> covered > 1)?(2):(1)):(0)) +	\
   ((p1 -> grappled)?(-2):(0)) +				\
   ((p1 -> prone)?(-2):(0)) +					\
   ((p1 -> control_turns > 0)?(-2):(0)) +			\
   ((p1 -> full_defense)?(2):(0)))
#define GET_DEFENSE_INT(p) ((RAND()%2)?					\
			    ((p -> dexterity + p -> essence +		\
			      GET_BONUS(p)) / 2):			\
			    ((p -> dexterity + p -> essence +		\
			      GET_BONUS(p) + 1) / 2))
#define GET_ATTACK_DICE(p) (p -> dexterity + p -> essence +	\
			    GET_BONUS(p))
#define GET_WITHERING_DICE(p) (p -> dexterity + p -> essence +	\
			       (p -> crippling_penalty) +	\
			    GET_BONUS(p) + p -> accuracy)
#define GET_STRENGTH_DICE(p) (p -> strength + p -> essence +	\
			      (p -> crippling_penalty) +	\
			      GET_BONUS(p))
#define GET_INTELIGENCE_DICE(p) (p -> inteligence + p -> essence +	\
				 GET_BONUS(p))
#define GET_PERCEPTION_DICE(p) (p -> perception + p -> essence +	\
				GET_BONUS(p) +				\
				((p -> blinded)?(-3):(0)))
#define GET_BONUS_ATTACK(p) (((p -> blinded)?(-3):(0)) +	\
			     (p -> crippling_penalty) +		\
			     ((p -> aiming > 0)?(+3):(0)) +	\
			     ((p -> prone)?(-3):(0)))
#define GET_BONUS_STEALTH(p) (((p -> blinded)?(-3):(0)) +	\
			      (p -> crippling_penalty) +	\
			      ((p -> prone)?(-3):(0)))
#define GET_BONUS_PERCEPTION(p) (((p -> blinded)?(-3):(0)) +	\
			      ((p -> prone)?(-3):(0)))
#define GET_BONUS_MOVE(p) (((p -> blinded)?(-3):(0)) + \
			   (p -> crippling_penalty) +		\
			   ((p -> fleet_of_foot)?(1):(0)) +	\
			   ((p -> difficult_terrain)?(-3):(0)))
#define GET_BONUS_DEFENSE(p1, p2) (MAX(GET_BONUS_PARRY(p1, p2),		\
				       GET_BONUS_EVASION(p1, p2)))
#define GET_BONUS_DEFENSE_INT(p1, p2)				\
  ((RAND()%2)?((int) GET_BONUS_DEFENSE(p1, p2)):		\
   ((int) (GET_BONUS_DEFENSE(p1, p2)+0.5)))
#define GET_RESIST_POISON(p) (p -> stamina + p -> essence +	\
			      GET_BONUS(p))
#define GET_SOCIAL_BONUS(p1, p2) ((p1 -> appearance > GET_RESOLVE(p2))?	\
				  (p1 -> appearance - GET_RESOLVE_INT(p2)): \
				  (0))
#define GET_JOIN_DICE(p) (p -> wits + p -> essence + GET_BONUS(p))

// Mínimo 1 dado rolado:
#define roll1(a) ((a<1)?(roll(1)):(roll(a)))

// Alcance
#define DEFAULT -1
#define CLOSE    0
#define RANGED   1

// Categorias de ataque
#define SPECIAL  1
#define PHYSICAL 2
#define MENTAL   3

// Shape
#define HEAD             1 
#define HEAD_WITH_LEGS   2
#define FISH             3
#define INSECTOID        4
#define QUADRUPED        5
#define FLYING_BUG       6
#define MULTIPLE_BODIES  7
#define OCTOPUS          8
#define HEAD_WITH_BASE   9
#define BIPEDAL         10
#define HUMANOID        11
#define BIRD            12
#define SERPENTINE      13
#define HEAD_WITH_ARMS  14


// Ações
#define WITHERING_ATTACK  1
#define DECISIVE_ATTACK   2
#define CLASH_WITHERING   3
#define CLASH_DECISIVE    4
#define FULL_DEFENSE      5
#define THREATEN          6
#define AIM               7
#define THROW_POISON      8
#define MAKE_SLEEP        9
#define SMOKE            10
#define CLEAN_EYES       11
#define DISENGAGE        12
#define ATT_DISENGAGE    13
#define AIM_ATT          14
#define TAKE_COVER       15
#define DIF_TERRAIN      16
#define GRAPPLE          17
#define RESTRAIN         18
#define SLAM             19
#define GET_UP           20
#define AMBUSH           21
#define HIDE_ATT         22
#define SCARE            23
#define ANGER            24
#define FRIENDSHIP       25
#define FLIRT            26
#define SEDUCE           27
#define TRANSFORM        28
#define PARALYZE         29
#define BURN             30
#define END_BURN         31
#define NUMBER_OF_MOVES  32

#define ATTACK_NAME(...) ATTACK_NAME3(__VA_ARGS__)
#define ATTACK_NAME3(name, type, ranged) name
// Attacks
#define STRUGGLE         "Se debater", NONE, false
#define TACKLE           "Investida", NORMAL, false
#define POISON_STING     "Picada Venenosa", POISON, true
#define PECK             "Bicada", FLYING, false
#define FURY_ATTACK      "Ataque de Fúria", NORMAL, false
#define DOUBLE_KICK      "Chute Duplo", FIGHTING, false
#define HORN_ATTACK      "Ataque de Chifres", NORMAL, false
#define POISON_JAB       "Apunhalada Venenosa", POISON, false
#define EARTH_POWER      "Poder da Terra", GROUND, true
#define MEGAHORN         "Mega-Chifrada", BUG, false
#define STORED_POWER     "Liberar Poder Acumulado", PSYCHIC, true
#define POUND            "Pancada", NORMAL, false
#define DISARMING_VOICE  "Voz Desarmante", FAIRY, true
#define METEOR_MASH      "Soco Meteórico", STEEL, false
#define MOONBLAST        "Força Lunar", FAIRY, true
#define QUICK_ATTACK     "Ataque Veloz", NORMAL, false
#define EMBER            "Brasa", FIRE, true
#define INCINERATE       "Incinerar", FIRE, true
#define EXTRASENSORY     "Extrasensorial", PSYCHIC, true
#define FLAMETHROWER     "Lança-Chamas", FIRE, true
#define INFERNO          "Inferno", FIRE, true
#define FIRE_BLAST       "Rajada de Fogo", FIRE, true
#define ECHOED_VOICE     "Voz Ecoada", NORMAL, true
#define COVET            "Cobiça", NORMAL, false
#define ROUND            "Ronda", NORMAL, true
#define BODY_SLAM        "Pancada de Corpo", NORMAL, false
#define HYPER_VOICE      "Hiper-Voz", NORMAL, true
#define PLAY_ROUGH       "Brincadeira Violenta", FAIRY, false
#define DOUBLE_EDGE      "Faca de Dois Gumes", NORMAL, false
#define VINE_WHIP        "Chicote de Vinhas", GRASS, false
#define RAZOR_LEAF       "Folhas-Gilete", GRASS, true
#define SEED_BOMB        "Bomba de Sementes", GRASS, true
#define PETAL_BLIZZARD   "Tempestade de Pétalas", GRASS, true
#define TAKE_DOWN        "Desmantelar", NORMAL, false
#define PETAL_DANCE      "Dança de Pétalas", GRASS, false
#define ABSORB           "Absorção", GRASS, true
#define ASTONISH         "Assustar", GHOST, false
#define AIR_CUTTER       "Ar Cortante", FLYING, true
#define BITE             "Mordida", DARK, false
#define VENOSHOCK        "Choque de Veneno", POISON, true
#define AIR_SLASH        "Corte de Ar", FLYING, true
#define LEECH_LIFE       "Sugar a Vida", BUG, false
#define POISON_FANG      "Presa Venenosa", POISON, false
#define ACID             "Ácido", POISON, true
#define MEGA_DRAIN       "Mega Sucção", GRASS, true
#define GIGA_DRAIN       "Giga Sucção", GRASS, true
#define FURY_SWIPES      "Garra Furiosa", NORMAL, false
#define SCRATCH          "Arranhar", NORMAL, false
#define SLASH            "Corte", NORMAL, false
#define X_SCISSOR        "Tesoura-X", BUG, false
#define PSYBEAM          "Raio Psíquico", PSYCHIC, true
#define PSYCHIC_ATT      "Ataque Psíquico", PSYCHIC, true
#define GUST             "Lufada de Vento", FLYING, true
#define CONFUSION        "Confusão", PSYCHIC, true
#define BUG_BUZZ         "Zumbido de Inseto", BUG, true
#define MUD_SLAP         "Tapa de Lama", GROUND, false
#define BULLDOZE         "Tremor", GROUND, true
#define SUCKER_PUNCH     "Soco Inesperado", DARK, false
#define EARTHQUAKE       "Terremoto", GROUND, true
#define NIGHT_SLASH      "Corte da Noite", DARK, false
#define TRI_ATTACK       "Tri-Ataque", NORMAL, true
#define FEINT            "Finta", NORMAL, true
#define PAY_DAY          "Dia do Pagamento", NORMAL, true
#define ASSURANCE        "Ataque de Garantia", DARK, false
#define POWER_GEM        "Gema do Poder", ROCK, true
#define WATER_GUN        "Pistola de Água", WATER, true
#define WATER_PULSE      "Pulso de Água", WATER, true
#define ZEN_HEADBUTT     "Cabeçada Zen", PSYCHIC, false
#define AQUA_TAIL        "Cauda de Água", WATER, false
#define HYDRO_PUMP       "Bomba de Água", WATER, true
#define AQUA_JET         "Aqua-Jato", WATER, false
#define DRAGON_BREATH    "Sopro do Dragão", DRAGON, true
#define FIRE_FANG        "Dentes de Fogo", FIRE, false
#define FLARE_BLITZ      "Bombardeio de Labaredas", FIRE, false
#define DRAGON_CLAW      "Garra do Dragão", DRAGON, false
#define HEAT_WAVE        "Onda de Calor", FIRE, true
#define KARATE_CHOP      "Golpe de Karatê", FIGHTING, false
#define U_TURN           "Giro em U", BUG, false
#define THRASH           "Espancamento", NORMAL, false
#define OUTRAGE          "Ultrage", DRAGON, false
#define RAGE             "Fúria", NORMAL, false
#define FLAME_WHEEL      "Roda de Fogo", FIRE, false
#define RETALIATE        "Retaliação", NORMAL, false
#define CRUNCH           "Mastigada", DARK, false
#define EXTREME_SPEED    "Velocidade Extrema", NORMAL, false
#define BURN_UP          "Chama Final", FIRE, true
#define MUD_SHOT         "Tiro de Lama", GROUND, true
#define BUBBLE_BEAM      "Raio de Bolhas", WATER, true
#define SUBMISSION       "Submissão", FIGHTING, false
#define DYNAMIC_PUNCH    "Soco Dinâmico", FIGHTING, false
#define PSYSHOCK         "Choque Psíquico", PSYCHIC, true
#define PSYCHO_CUT       "Corte Psíquico", PSYCHIC, false
#define FUTURE_SIGHT     "Visão do Futuro", PSYCHIC, true
#define THUNDER_SHOCK    "Choque do Trovão", ELECTRIC, false
#define SPARK            "Fagulhas", ELECTRIC, false
#define FLASH_CANNON     "Canhão de Luz", STEEL, true
#define DISCHARGE        "Descarregar", ELECTRIC, true
#define ZAP_CANNON       "Canhão Eletromagnético", ELECTRIC, true
#define ICE_SHARD        "Fragmento de Gelo", ICE, true
#define AURORA_BEAM      "Raio da Aurora", ICE, true
#define RAZOR_SHELL      "Concha Gilete", WATER, false
#define ICE_BEAM         "Raio de Gelo", ICE, true
#define ICICLE_SPEAR     "Lança de Gelo", ICE, true
#define ICICLE_CRASH     "Colisão de Gelo", ICE, true
#define METAL_CLAW       "Garra de Metal", STEEL, false
#define STOMP            "Pisotear", NORMAL, false
#define CRABHAMMER       "Martelo de Caranguejo", WATER, false
#define HAMMER_ARM       "Martelada de Braço", FIGHTING, false
#define DAZZLING_GLEAM   "Clarão Deslumbrante", FAIRY, true
#define ROCK_BLAST       "Explosão de Rocha", ROCK, true
#define ROLLOUT          "Rolar", ROCK, false
#define ANCIENT_POWER    "Poder Antigo", ROCK, true
#define BRINE            "Salmoura", WATER, true
#define SURF             "Surfe", WATER, true
#define LIQUIDATION      "Liquidação", WATER, false
#define STONE_EDGE       "Lâminas de Pedra", ROCK, true
#define BULLET_PUNCH     "Soco-Bala", STEEL, false
#define VACUUM_WAVE      "Onda de Vácuo", FIGHTING, true
#define MACH_PUNCH       "Soco Rápido", FIGHTING, false
#define POWER_UP_PUNCH   "Soco Empoderador", FIGHTING, false
#define REVENGE          "Vingança", FIGHTING, false
#define THUNDER_PUNCH    "Soco do Trovão", ELECTRIC, false
#define FIRE_PUNCH       "Soco de Fogo", FIRE, false
#define ICE_PUNCH        "Soco de Gelo", ICE, false
#define DRAIN_PUNCH      "Soco Drenagem", FIGHTING, false
#define MEGA_PUNCH       "Mega Soco", NORMAL, false
#define CLOSE_COMBAT     "Combate Corpo-a-Corpo", FIGHTING, false
#define FOCUS_PUNCH      "Soco Focado", FIGHTING, false
#define PLUCK            "Pegar com Bico", FLYING, false
#define DRILL_PECK       "Bicada-Furadeira", FLYING, false
#define THUNDER          "Trovão", ELECTRIC, true
#define SWIFT            "Ataque Cadente", NORMAL, true
#define AURA_SPHERE      "Esfera da Aura", FIGHTING, true
#define PSYSTRIKE        "Impacto Psíquico", PSYCHIC, true
#define HEADBUTT         "Cabeçada", NORMAL, false
#define DREAM_EATER      "Comer Sonhos", PSYCHIC, true
#define BULLET_SEED      "Rajada de Sementes", GRASS, true
#define UPROAR           "Gritaria", NORMAL, true
#define WOOD_HAMMER      "Martelada de Madeira", GRASS, false
#define LEAF_STORM       "Tempestade de Folhas", GRASS, true
#define SMOG             "Fumaça", POISON, true
#define CLEAR_SMOG       "Fumaça Clara", POISON, true
#define SLUDGE           "Ataque de Lama", POISON, true
#define SLUDGE_BOMB      "Bomba de Lama", POISON, true
#define BELCH            "Arroto", POISON, true
#define EXPLOSION        "Explosão", NORMAL, true
#define DOUBLE_HIT       "Golpe Duplo", NORMAL, false
#define TWISTER          "Ciclone", DRAGON, true
#define WING_ATTACK      "Ataque de Asa", FLYING, false
#define RAZOR_WIND       "Vento-Gilete", NORMAL, true
#define DRAGON_PULSE     "Pulso do Dragão", DRAGON, true
#define LAVA_PLUME       "Bruma de Lava", FIRE, true
#define HYPER_BEAM       "Hiper-Raio", NORMAL, true
#define ROCK_THROW       "Jogar Pedra", ROCK, true
#define ROCK_SLIDE       "Deslizamento de Pedra", ROCK, true
#define SMACK_DOWN       "Derrubada", ROCK, true
#define STEAMROLLER      "Rolo Compressor", BUG, false
#define FURY_CUTTER      "Corte Furioso", BUG, false
#define FALSE_SWIPE      "Corte Falso", NORMAL, false
#define CUT              "Corte", NORMAL, false
#define LEAF_BLADE       "Lâmina de Folha", GRASS, false
#define AERIAL_ACE       "Ás Aéreo", FLYING, false
#define KNOCK_OFF        "Derrubar", DARK, false
#define BRAVE_BIRD       "Pássaro Bravo", FLYING, false
#define DRILL_RUN        "Corrida Furadeira", GROUND, false
#define LAST_RESORT      "Último Recurso", NORMAL, false
#define MUDDY_WATER      "Água Lamacenta", WATER, true
#define PIN_MISSILE      "Míssil de Espinhos", BUG, true
#define THUNDER_FANG     "Dentes do Trovão", ELECTRIC, false
#define HURRICANE        "Furacão", FLYING, true
#define THUNDERBOLT      "Raio do Trovão", ELECTRIC, true
#define RAPID_SPIN       "Giro Rápido", NORMAL, false
#define SHOCK_WAVE       "Onda de Choque", ELECTRIC, true
#define GIGA_IMPACT      "Giga-Impacto", NORMAL, false
#define IRON_TAIL        "Cauda de Ferro", STEEL, false
#define POWER_WHIP       "Chicote do Poder", GRASS, false
#define VISE_GRIP        "Aperto de Garra", NORMAL, false
#define STORM_THROW      "Arremesso da Tempestade", FIGHTING, false
#define BUG_BITE         "Mordida de Inseto", BUG, false
#define SUPERPOWER       "Superpoder", FIGHTING, false
#define STRENGTH         "Demonstração de Força", NORMAL, false
#define FLAME_CHARGE     "Investida de Chamas", FIRE, false
#define SMART_STRIKE     "Chifrada Certeira", STEEL, false
#define ICE_FANG         "Presas de Gelo", ICE, false
#define WATERFALL        "Cachoeira", WATER, false
#define HEX              "Feitiço", GHOST, true
#define SLUDGE_WAVE      "Onda de Lama", POISON, true
#define LICK             "Lambida", GHOST, false
#define DRAGON_RUSH      "Investida de Dragão", DRAGON, false
#define DUAL_CHOP        "Golpe Duplo", DRAGON, false
#define LOW_SWEEP        "Rasteira", FIGHTING, false
#define VITAL_THROW      "Arremesso Vital", FIGHTING, false
#define CROSS_CHOP       "Golpe Cruzado", FIGHTING, false
#define JUMP_KICK        "Salto Pontapé", FIGHTING, false
#define IRON_HEAD        "Cabeçada de Ferro", STEEL, false
#define BRICK_BREAK      "Quebra Tijolo", FIGHTING, false
#define BLAZE_KICK       "Chute Flamejante", FIRE, false
#define HIGH_JUMP_KICK   "Chute Super-Voadora", FIGHTING, false
#define MEGA_KICK        "Mega-Chute", NORMAL, false
#define PAYBACK          "Revide", DARK, false
#define BONE_RUSH        "Fúria de Ossos", GROUND, true
#define BOOMERANG        "Bumerangue", GROUND, true
#define STOMPING_TANTRUM "Pisoteio da Cólera", GROUND, false
#define POWDER_SNOW      "Pó de Neve", ICE, true
#define BLIZZARD         "Nevasca", ICE, true
#define FREEZE_DRY       "Congelamento Seco", ICE, true
#define SNORE            "Ronco", NORMAL, true
#define HIGH_HORSEPOWER  "100.000 Cavalo-Vapor", GROUND, false
#define SHADOW_PUNCH     "Soco das Sombras", GHOST, false
#define SHADOW_BALL      "Esfera de Sombra", GHOST, true
#define DARK_PULSE       "Pulso Sombrio", DARK, true
#define FELL_STINGER     "Ferrão Letal", BUG, false
#define ACID_SPRAY       "Esguicho Ácido", POISON, true
#define GUNK_SHOT        "Tiro de Sujeira", POISON, true
#define STRUGGLE_BUG     "Ira de Inseto", BUG, true
#define CROSS_POISON     "Corte Venenoso", POISON, false
#define LEAF_TORNADO     "Tornado de Folhas", GRASS, true
#define LUNGE            "Estocada", BUG, false
#define ICY_WIND         "Vento Gélido", ICE, true
#define CHARGE_BEAM      "Carga de Raio", ELECTRIC, true
#define NUZZLE           "Chamego Elétrico", ELECTRIC, false

// Attack and disengage:
static char uturn[16] = "Giro em U";

// Full defense moves:
static char protect[16] = "Proteção";
static char detect[16] = "Detecção";

// Smoke moves:
static char smokescreen[32] = "Cortina de Fumaça";
static char sand_attack[32] = "Ataque de Areia";

// Aim+Attack moves
static char solar_beam[16] = "Raio Solar";
static char skull_bash[32] = "Cabeçada Propelida";
static char sky_attack[32] = "Ataque Celeste";

// Hide+Attack moves
static char dig[8] = "Cavar";
static char dive[16] = "Mergulhar";

// Make sleep moves
static char sleep_powder[16] = "Pó do Sono";
static char hypnosis[16] = "Hipnose";
static char sing[8] = "Cantar";
static char yawn[8] = "Bocejar";

// Threaten moves:
static char roar[8] = "Rugido";

// Scare moves
static char noble_roar[16] = "Rugido Nobre";
static char scary_face[32] = "Cara Assustadora";
static char leer[32] = "Olhar Fulminante";

// Throw poison moves
static char poison_powder[16] = "Pó Venenoso";
static char poison_gas[16] = "Gás Venenoso";
static char toxic[8] = "Tóxico";

// Take Cover Moves
static char light_screen[16] = "Tela de Luz";

// Difficult terrain moves
static char stealth_rock[16] = "Pedras Ocultas";
static char spikes[16] = "Espinhos";
static char toxic_spikes[32] = "Espinhos Tóxicos";

// Grapple moves
static char bind[8] = "Apertar";
static char fire_spin[32] = "Redemoinho de Fogo";
static char sand_tomb[32] = "Inferno de Areia";
static char whirlpool[16] = "Redemoinho";
static char wrap[16] = "Envolver";
static char glare[32] = "Olhar da Serpente";

// Slam moves
static char slam[32] = "Jogar contra o Chão";
static char circle_throw[32] = "Arremesso Circular";
static char dragon_tail[32] = "Cauda de Dragão";
static char whirlwind[32] = "Rajada de Vento";
static char seismic_toss[32] = "Arremesso Sísmico";

// Ambush moves
static char fake_out[16] = "Tapa nas Mãos";

// Anger move
static char taunt[16] = "Provocação";
static char swagger[16] = "Arrogância";

// Friendship move
static char growl[16] = "Grunhido Fofo";
static char fake_tears[32] = "Lágrimas Falsas";
static char play_nice[16] = "Propor Amizade";
static char tickle[16] = "Fazer Cócegas";
static char tail_whip[16] = "Abanar o Rabo";

// Flirt move
static char baby_doll_eyes[32] = "Olhar de Boneca Bebê";
static char captivate[8] = "Cativar";
static char sweet_kiss[16] = "Beijo Doce";
static char charm[16] = "Encantar";

// Seduce moves
static char lovely_kiss[16] = "Beijo Amável";

// Aim move
static char lock_on[8] = "Mirar";
static char laser_focus[8] = "Aguçar";

// Paralyze move
static char thunder_wave[16] = "Onda do Trovão";
static char stun_spore[16] = "Pó Atordoante";

// Burn move
static char will_o_wisp[16] = "Fogo Fátuo";

// Responde se uma ação é ataque:
#define IS_ATTACK(x) (x == WITHERING_ATTACK || x == DECISIVE_ATTACK ||	\
		      x == CLASH_DECISIVE || x == CLASH_WITHERING ||	\
		      x == ATT_DISENGAGE)

// Tamanhos
#define TINY      1
#define NORMAL    2
#define LEGENDARY 3

// Status
#define PARALYSIS 2
#define CRIPPLE   3

// Tipos
#define NONE      -1 
#define BUG       3
#define PSYCHIC   4
#define GRASS     5
#define POISON    6
#define FIRE      7
#define FIGHTING  8
#define WATER     9
#define FLYING   10
#define ELECTRIC 11
#define GROUND   12
#define ROCK     13
#define ICE      14
#define DRAGON   15
#define GHOST    16
#define DARK     17
#define STEEL    18
#define FAIRY    19
char string_type[20][20] = {"Sem Tipo", "Sem Tipo", "Normal", "Inseto",
  "Psíquico",
  "Grama", "Venenoso", "Fogo", "Lutador", "Água", "Voador", "Elétrico",
  "Terrestre", "Pedra", "Gelo", "Dragão", "Fantasma", "Sombrio", "Metal", "Fada"};

struct _Wrng *RNG;

#define RAND() _Wrand(RNG)

#define BEGIN_DESCRIPTION(name) { int _d = 0; struct pokemon *_p = name
#define DESCRIPTION(...) DESCRIPTION3(__VA_ARGS__)
#define DESCRIPTION3(a, b, c)					\
  _p -> description[_d] = (char *) malloc(strlen(a)+1);		\
  strcpy(_p -> description[_d], a); _p -> type[_d] = b;		\
  _p -> ranged[_d] = c;						\
  _d ++
#define END_DESCRIPTION() for(; _d < 26; _d ++) _p -> description[_d] = NULL; }

struct pokemon{
  int onslaugh_penalty;
  char name[16], desc[64];
  int vitality;
  int extra;
  int penalty[32];
  int crippling_penalty;
  char *description[26];
  bool ranged[26];
  int type[26];
  int damage;
  int essence;
  int strength, dexterity, stamina;
  int charisma, manipulation, appearance;
  int inteligence, wits, perception;
  int willpower;
  int type1, type2;
  int accuracy, physical_damage, special_damage, physical_soak, special_soak;
  int team;
  int shape;
  int initiative;
  int action;
  int size;
  bool broken_leg;
  int turns_since_last_initiative_crash;
  int turns_crashed;
  bool extra_turn, full_defense, crashed_by_its_actions;
  char *burn_move;
  char *aim_move; int aiming;
  char *full_defense_move;
  char *threaten_move; bool resisted_threat[3]; bool forfeit;
  char *seduce_move;
  char *poison_move; int poison_remaining, poisoned;
  bool burned;
  char *paralysis_move; int paralysis_remaining, paralyzed;
  char *sleep_move; bool making_sleep; int sleepy; int motes; int in_the_mist;
  // Other senses: Supersonic
  char *smoke_move; bool blinded; bool other_senses; int distance;
  bool hideous; // ...
  bool can_teleport;
  bool mental_attack; // Psyshock, Psystrike
  char *att_disengage_move; int att_disengage_type;
  bool att_disengage_ranged;
  char *aim_att_move; int aim_att_type; bool aim_att_ranged;
  char *hide_att_move; int hide_att_type; bool hiding;
  char *take_cover_move; int covered; int cover_duration;
  char *dif_terrain_move; bool difficult_terrain;
  char *grapple_move; int grapple_type; int control_turns; bool grappled;
  int grapple_category;
  bool restrained;
  bool ia;
  char *slam_move; bool prone; int slam_type;
  bool first_turn;
  char *ambush_move; int ambush_type; int ambush_attack;
  char *scare_move; int afraid;
  char *anger_move; int angry;
  char *friendship_move, *friendship2, *friendship3; int friendly;
  char *flirt_move; char *flirt2; int infatuated; bool genderless;
  bool can_transform;
  bool enabled_moves[NUMBER_OF_MOVES];
  bool fleet_of_foot;
};

struct pokemon *new_base_pokemon(int team){
  int i;
  struct pokemon *p = (struct pokemon *) malloc(sizeof(struct pokemon));
  for(i = 0; i < 32; i ++)
    p ->  penalty[i] = -100;
  for(i = 0; i < 26; i ++){
    p -> type[i] = NONE;
    p -> ranged[i] = false;
    p -> description[i] = NULL;
  }
  p ->  crippling_penalty = 0;
  p -> broken_leg = false;
  p ->  accuracy = p -> physical_damage = p ->  special_damage = 0;
  p -> physical_soak = p ->  special_soak = 0;
  p -> damage = 0;
  p -> turns_since_last_initiative_crash = -1;
  p -> turns_crashed = -1;
  p -> crashed_by_its_actions = false;
  p -> extra_turn = false;
  p -> full_defense = false;
  p -> full_defense_move = NULL;
  p -> action = NONE;
  p -> aim_move = NULL;
  p -> aiming = 0;
  p ->  threaten_move = NULL;
  p -> seduce_move = NULL;
  p ->  resisted_threat[0] = false;
  p ->  resisted_threat[1] = false;
  p ->  resisted_threat[2] = false;
  p -> forfeit = false;
  p -> onslaugh_penalty = 0;
  p -> poison_move = NULL; p ->  poison_remaining = 0; p -> poisoned = 0;
  p -> paralysis_move = NULL; p ->  paralysis_remaining = 0; p -> paralyzed = 0;
  p -> sleep_move = NULL; p -> making_sleep = false; p ->  sleepy = false;
  p -> in_the_mist = 0; p ->  motes = 0;
  p -> smoke_move = NULL; p -> blinded = false; p -> other_senses = false;
  p -> distance = 0;
  p -> hideous = false;
  p -> can_teleport = false;
  p -> mental_attack = false;
  p -> burn_move = NULL;
  p -> att_disengage_move = NULL;
  p -> att_disengage_type = NONE;
  p -> att_disengage_ranged = false;
  p -> aim_att_move = NULL;
  p -> aim_att_type = NONE;
  p -> aim_att_ranged = false;
  p -> hide_att_move = NULL;
  p -> hide_att_type = NONE;
  p -> hiding = false;
  p -> burned = false;
  p -> take_cover_move = NULL;
  p ->  covered = 0;
  p ->  cover_duration = 0;
  p -> dif_terrain_move = NULL;
  p ->  difficult_terrain = false;
  p -> grapple_move = NULL;
  p -> grapple_type = NONE;
  p -> grapple_category = PHYSICAL;
  p -> control_turns = 0;
  p -> grappled = false;
  p -> restrained = false;
  p -> ia = true;
  p -> slam_move = NULL;
  p -> prone = false;
  p ->  slam_type = NONE;
  p ->  first_turn = true;
  p -> ambush_move = NULL;
  p ->  ambush_type = NONE;
  p -> ambush_attack = NONE;
  p -> scare_move = NULL;
  p -> afraid = 0;
  p -> anger_move = NULL;
  p -> angry = 0;
  p -> friendship_move = p -> friendship2 = p -> friendship3 = NULL;
  p -> friendly = 0;
  p -> flirt_move = p -> flirt2 = NULL;
  p ->  infatuated = 0;
  p -> genderless = false;
  p -> can_transform = false;
  p -> fleet_of_foot = false;
  return p;
}

int roll(int dice_pool){
  int result = 0, i, value;
  bool success = false, botch = false;
  for(i = 0; i < dice_pool; i ++){
    do{
      value = RAND() % 16;
    }while(value  >= 10);
    if(value  == 0){
      success = true;
      result += 2;
    }
    else if(value >= 7){
      success = true;
      result ++;
    }
    else if(value == 1)
      botch = true;
  }
  if(botch && !success)
    return -1;
  else
    return result;
}

int roll_no_double(int dice_pool){
  int result = 0, i, value;
  bool success = false, botch = false;
  for(i = 0; i < dice_pool; i ++){
    do{
      value = RAND() % 16;
    }while(value  >= 10);
    if(value >= 7 || value == 0){
      success = true;
      result ++;
    }
    else if(value == 1)
      botch = true;
  }
  if(botch && !success)
    return -1;
  else
    return result;
}

int GET_ATTACK_TYPE(struct pokemon *p1, struct pokemon *p2){
  int physical = p1 -> strength - p2 -> stamina + p1 -> physical_damage -
    p2 -> physical_soak;
  int special = p1 -> inteligence - p2 -> wits + p1 -> special_damage -
    p2 -> special_soak;
  int mental = p1 -> inteligence - p2 -> stamina + p1 -> special_damage -
    p2 -> physical_soak;
  if(p1 -> distance > 0 || p2 -> distance > 0)
    physical = -1000;
  else if(p1 -> covered > 0)
    physical = - (p1 -> covered) * 100;
  if(p1 -> mental_attack == false)
    mental = -1000;
  if(p2 -> covered == 3)
    special = -1000;
  if(physical == -1000 && special == -1000 && mental == -1000)
    return NONE;
  if(physical == special){
    if(p1 -> strength > p1 -> inteligence)
      special --;
    else if(p1 -> strength < p1 -> inteligence)
      physical --;
    else if(RAND() % 2)
      special --;
    else
      physical --;
  }
  if(physical == mental){
    if(p1 -> strength > p1 -> inteligence)
      mental --;
    else if(p1 -> strength < p1 -> inteligence)
      physical --;
    else if(RAND() % 2)
      mental --;
    else
      physical --;
  }
  if(special == mental){
    if(RAND() % 2)
      mental --;
    else
      special --;
  }
  if(p1 -> control_turns && p1 -> grapple_move == glare)
    return SPECIAL;
  else if(physical >= special && physical >= mental)
    return PHYSICAL;
  else if(special >= mental)
    return SPECIAL;
  else
    return MENTAL;
}

int ROLL_ATTACK(struct pokemon *p1, struct pokemon *p2, int extra,
		bool *legendary){
  int attack;
  switch(GET_ATTACK_TYPE(p1, p2)){
  case PHYSICAL:
    if(p1 -> covered > 2)
      attack = p1 -> strength - p2 -> stamina + extra - 2 +
	p1 -> physical_damage - p2 -> physical_soak;
    else
      attack = p1 -> strength - p2 -> stamina + extra - p1 -> covered +
	p1 -> physical_damage - p2 -> physical_soak;
    break;
  case SPECIAL:
    attack = p1 -> inteligence - p2 -> wits + extra + p1 -> special_damage -
      p2 -> special_soak;
    break;
  case MENTAL:
    attack = p1 -> inteligence - p2 -> stamina + extra + p1 -> special_damage -
      p2 -> physical_soak;
    break;
  default:
    return 0;
  }
  if(legendary != NULL)
    *legendary = (attack >= 10);
  return roll1(attack);
}


void steal_initiative(struct pokemon *p1, struct pokemon *p2, int value,
		      bool legendary){
  int initial1 = p1 -> initiative;
  int initial2 = p2 -> initiative;
  p1 -> initiative += value + 1;
  p2 -> initiative -= value;
  if(p2 -> size == LEGENDARY && p1 -> size < LEGENDARY && initial2 > 0 &&
     p2 -> initiative <= 0 && !legendary)
    p2 -> initiative = 1;
  if(initial2 > 0 && p2 -> initiative <= 0){ // Crashed oponent
    printf("%s teve sua Iniciativa quebrada!\n", p2 -> name);
    p2 -> turns_crashed = 0;
    p2 -> crashed_by_its_actions = false;
    if(p2 -> turns_since_last_initiative_crash == -1 ||
       p2 -> turns_since_last_initiative_crash > 1){
      p1 -> initiative += 5;
    }
    if(initial1 < 0 && !(p1 -> crashed_by_its_actions)){ // Initiative shift
      if(p1 -> initiative < 3)
	p1 -> initiative = 3;
      p1 -> initiative += roll(GET_JOIN_DICE(p1));
      p1 -> extra_turn = true;
    }
  }
  if(initial1 <= 0 && p1 -> initiative > 0){ // Left crash
    printf("%s não está mais com Iniciativa quebrada.\n", p1 -> name);
    p1 -> turns_since_last_initiative_crash = 0;
    p1 -> crashed_by_its_actions = false;
    p1 -> turns_crashed = -1;
  }
}

void add_initiative(struct pokemon *p1, struct pokemon *p2, int value,
		    bool fault){
  int initial1 = p1 -> initiative;
  int initial2 = p2 -> initiative;
  p1 -> initiative += value;
  if(initial1 <= 0 && p1 -> initiative > 0){ // Left crash
    printf("%s não está mais com Iniciativa quebrada.\n", p1 -> name);
    p1 -> turns_since_last_initiative_crash = 0;
    p1 -> crashed_by_its_actions = false;
    p1 -> turns_crashed = -1;
  }
  else if(initial1 > 0 && p1 -> initiative <=0){
    printf("%s quebrou sua própria Iniciativa!\n", p1 -> name);
    p1 -> crashed_by_its_actions = fault;
    p1 -> turns_crashed = 0;
    p1 -> initiative -= 5;
    p2 -> initiative += 5;
  }
  if(initial2 <= 0 && p2 -> initiative > 0){ // Left crash
    printf("%s não está mais com Iniciativa quebrada.\n", p2 -> name);
    p2 -> turns_since_last_initiative_crash = 0;
    p2 -> crashed_by_its_actions = false;
    p2 -> turns_crashed = -1;
  }
}

void pokemon_update_turn(struct pokemon *p, struct pokemon *oponent){
  if(p -> making_sleep && p -> action != MAKE_SLEEP){
    p -> motes -= 3;
    if(p -> motes <= 0){
      p -> motes = 0;
      p -> making_sleep = false;
      if(p -> sleep_move == sleep_powder)
	printf("O Pó do Sono se dispersou completamente pelo vento.\n");
      else if(p -> sleep_move == hypnosis)
	printf("%s perdeu sua concentração.\n", p -> name);
      else if(p -> sleep_move == sing)
	printf("%s interrompeu sua melodia.\n", p -> name);
      else if(p -> sleep_move == yawn)
	printf("%s falhou em fingir estar com sono.\n", p -> name);
      p -> sleep_move = NULL;
    }
    else{
      float total = ((p -> initiative > 0)?(7.0):(10.0));
      if(p -> sleep_move == sleep_powder)
	printf("O vento está dispersando o Pó do Sono. Há %d%% do necessário.\n",
	       (int) (100 * (p -> motes / total)));
      else if(p -> sleep_move == hypnosis)
	printf("%s está perdendo concentração. Há %d%% do necessário.\n",
	       p -> name,
	       (int) (100 * (p -> motes / total)));
      else if(p -> sleep_move == sing)
	printf("%s está perdendo concentração na música. "
	       "Há %d%% do necessário.\n",
	       p -> name,
	       (int) (100 * (p -> motes / total)));
      else if(p -> sleep_move == yawn)
	printf("%s está parando de fingir estar com sono. "
	       "%s foi %d%% afetado.\n",
	       p -> name, oponent -> name,
	       (int) (100 * (p -> motes / total)));

    }
    GETCHAR();
  }
  if(p -> action != AIM_ATT && p -> action != HIDE_ATT)
    p -> action = NONE;
  if(p -> turns_since_last_initiative_crash >= 0)
    p -> turns_since_last_initiative_crash ++;
  if(p -> turns_crashed >= 0){
    p -> turns_crashed ++;
    if(p -> turns_crashed >= 4){
      p -> turns_crashed = -1;
      p -> crashed_by_its_actions = false;
      p -> initiative = 3;
      p -> turns_since_last_initiative_crash = 0;
    }
  }
  if(p -> aiming > 0)
    p -> aiming --;
  if(p -> poisoned)
    p -> poisoned --;
  if(p -> paralyzed)
    p -> paralyzed --;
  if(p -> burned){
    int damage = roll(1);
    printf("%s está queimando e sofreu %d de dano na Vitalidade.\n",
	   p -> name, damage);
    p -> damage += damage;
    if(p -> damage > p -> vitality)
      printf("%s MORREU!!!! \n", p -> name);
    GETCHAR();
  }
  if(p -> poisoned){
    int damage = roll(2);
    if(damage < 0)
      damage = 0;
    if(p -> initiative > 0){
      printf("%s está envenenado e sofreu %d de dano na Iniciativa.\n",
	     p -> name, damage);
      add_initiative(p, oponent, -damage, false);
    }
    else{
      printf("%s está envenenado e sofreu %d de dano.\n", p -> name, damage);
      p -> damage += damage;
      if(p -> damage > p -> vitality)
	printf("%s MORREU!!!! \n", p -> name);
    }
    GETCHAR();
  }
  if(p -> paralyzed){
    int damage = roll(2);
    if(damage < 0)
      damage = 0;
    if(p -> initiative > 0){
      printf("%s está paralisado e perdeu %d de  Iniciativa.\n",
	     p -> name, damage);
      add_initiative(p, oponent, -damage, false);
    }
    else{
      printf("%s está paralisado e não pode usar ações de movimento.\n",
	     p -> name);
    }
    GETCHAR();
  }
  if(p -> in_the_mist){
    int s = roll(GET_RESIST_POISON(p));
    int duration = 7 - 3;
    if(p -> sleepy == 0 && duration < 3)
      duration = 3;
    p -> sleepy += duration;
    p -> in_the_mist --;
  }
  if(p -> sleepy){
    int damage = roll(3);
    if(damage < 0)
      damage = 0;
    printf("%s perdeu %d Iniciativa devido ao sono.\n", p -> name, damage);
    add_initiative(p, oponent, -damage, false);
    if(p -> initiative <= 0){
      printf("%s entrou em um sono profundo e não pode mais lutar.\n", p -> name);
      p -> forfeit = true;
    }
    p -> sleepy --;
    GETCHAR();
  }
  if(p -> cover_duration){
    p -> cover_duration --;
    if(p -> cover_duration == 0){
      printf("%s desapareceu.\n", p -> take_cover_move);
      p -> covered = 0;
    }
  }
  p -> full_defense = false;
}

int compute_weakness(int attack, struct pokemon *p2){
  int hardness = 0;
  switch(attack){
  case NORMAL:
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness += 1000;
    break;
  case FIRE:
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == ICE || p2 -> type2 == ICE) hardness --;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness --;
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness ++;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness ++;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness ++;
    break;
  case FIGHTING:
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness --;
    if(p2 -> type1 == ICE || p2 -> type2 == ICE) hardness --;
    if(p2 -> type1 == NORMAL || p2 -> type2 == NORMAL) hardness --;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness --;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness --;
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness ++;
    if(p2 -> type1 == FAIRY || p2 -> type2 == FAIRY) hardness ++;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness ++;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness ++;
    if(p2 -> type1 == PSYCHIC || p2 -> type2 == PSYCHIC) hardness ++;
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness += 1000;
    break;
  case WATER:
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness --;
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness --;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness --;
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness ++;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness ++;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness ++;
    break;
  case FLYING:
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness --;
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == ELECTRIC || p2 -> type2 == ELECTRIC) hardness ++;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    break;
  case GRASS:
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness --;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness --;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness --;
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness ++;
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness ++;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness ++;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness ++;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    break;
  case POISON:
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness ++;
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness ++;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness ++;
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness ++;
    break;
  case ELECTRIC:
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness --;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness --;
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness --;
    if(p2 -> type1 == ELECTRIC || p2 -> type2 == ELECTRIC) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness += 1000;
    break;
  case GROUND:
    if(p2 -> type1 == ELECTRIC || p2 -> type2 == ELECTRIC) hardness --;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness --;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness --;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness --;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness --;
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness ++;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness ++;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness += 1000;
    break;
  case PSYCHIC:
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness --;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness --;
    if(p2 -> type1 == PSYCHIC || p2 -> type2 == PSYCHIC) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness += 1000;
    break;
  case ROCK:
    if(p2 -> type1 == BUG || p2 -> type2 == BUG) hardness --;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness --;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness --;
    if(p2 -> type1 == ICE || p2 -> type2 == ICE) hardness --;
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness ++;
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    break;
  case ICE:
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness --;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == GROUND || p2 -> type2 == GROUND) hardness --;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == ICE || p2 -> type2 == ICE) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness ++;
    break;
  case BUG:
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness --;
    if(p2 -> type1 == GRASS || p2 -> type2 == GRASS) hardness --;
    if(p2 -> type1 == PSYCHIC || p2 -> type2 == PSYCHIC) hardness --;
    if(p2 -> type1 == FAIRY || p2 -> type2 == FAIRY) hardness ++;
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness ++;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == FLYING || p2 -> type2 == FLYING) hardness ++;
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness ++;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    break;
  case DRAGON:
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness --;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    if(p2 -> type1 == FAIRY || p2 -> type2 == FAIRY) hardness += 1000;
    break;
  case GHOST:
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness --;
    if(p2 -> type1 == PSYCHIC || p2 -> type2 == PSYCHIC) hardness --;
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness ++;
    if(p2 -> type1 == NORMAL || p2 -> type2 == NORMAL) hardness += 1000;
    break;
  case DARK:
    if(p2 -> type1 == GHOST || p2 -> type2 == GHOST) hardness --;
    if(p2 -> type1 == PSYCHIC || p2 -> type2 == PSYCHIC) hardness --;
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness ++;
    if(p2 -> type1 == FAIRY || p2 -> type2 == FAIRY) hardness ++;
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness ++;
    break;
  case STEEL:
    if(p2 -> type1 == FAIRY || p2 -> type2 == FAIRY) hardness --;
    if(p2 -> type1 == ICE || p2 -> type2 == ICE) hardness --;
    if(p2 -> type1 == ROCK || p2 -> type2 == ROCK) hardness --;
    if(p2 -> type1 == ELECTRIC || p2 -> type2 == ELECTRIC) hardness ++;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    if(p2 -> type1 == WATER || p2 -> type2 == WATER) hardness ++;
    break;
  case FAIRY:
    if(p2 -> type1 == DARK || p2 -> type2 == DARK) hardness --;
    if(p2 -> type1 == DRAGON || p2 -> type2 == DRAGON) hardness --;
    if(p2 -> type1 == FIGHTING || p2 -> type2 == FIGHTING) hardness --;
    if(p2 -> type1 == FIRE || p2 -> type2 == FIRE) hardness ++;
    if(p2 -> type1 == POISON || p2 -> type2 == POISON) hardness ++;
    if(p2 -> type1 == STEEL || p2 -> type2 == STEEL) hardness ++;
    break;
  }
  return hardness;
}


void check_valid_moves(struct pokemon *p, struct pokemon *oponent){
  int i;
  bool ambush_possible;
  int a1, a2;
  a1 = roll(GET_ATTACK_DICE(p));
  a2 = roll(GET_PERCEPTION_DICE(p));
  if(a1 == a2){
    if(RAND()%2)
      a1 --;
    else
      a2 --;
  }
  i = (p -> initiative - 1) / 2;
  if(i > 25) i = 25;
  while(p -> description[i] == NULL)
    i --;
  // Vendo quais golpes são possíveis:
  p -> enabled_moves[WITHERING_ATTACK] = (!(p -> restrained) &&
					  p -> infatuated < 2 &&
					  p -> afraid < 2 &&
					  !(oponent -> hiding) &&
					  GET_WITHERING_DICE(p) +
					  GET_BONUS_ATTACK(p) > 0);
  p -> enabled_moves[DECISIVE_ATTACK] = (!(p -> restrained) &&
					 p -> infatuated < 2 &&
					 p -> description[1] != NULL &&
					 p -> afraid < 2 &&
					 p -> friendly < 2 &&
					 !(oponent -> hiding) &&
					 GET_ATTACK_DICE(p) +
					 GET_BONUS_ATTACK(p) > 0 &&
					 p -> initiative > 0 &&
					 ((!(p -> ranged[i]) &&
					  p -> distance == 0 &&
					  oponent -> distance == 0) ||
					  (p -> ranged[i] &&
					   oponent -> covered != 3)));
  p -> enabled_moves[CLASH_WITHERING] = (!(p -> restrained) &&
					 p -> infatuated < 2 &&
					 p -> afraid < 2 &&
					 p -> angry < 2 &&
					 !(oponent -> hiding) &&
					 p -> initiative >
					 oponent -> initiative &&
					 oponent -> action == NONE &&
					 GET_WITHERING_DICE(p) +
					  GET_BONUS_ATTACK(p) > 0 &&
					 p -> initiative >= 2);
  p -> enabled_moves[CLASH_DECISIVE] =  (!(p -> restrained) &&
					 p -> infatuated < 2 &&
					 p -> afraid < 2 &&
					 p -> friendly < 2 &&
					 p -> angry < 2 &&
					 !(oponent -> hiding) &&
					 GET_ATTACK_DICE(p) +
					 GET_BONUS_ATTACK(p) > 0 &&
					 p -> enabled_moves[CLASH_WITHERING] &&
					 p -> enabled_moves[DECISIVE_ATTACK]);
  p -> enabled_moves[AIM] = (!(p -> restrained) &&
			     p -> angry < 2 &&
			     p -> afraid < 2 &&
			     !(oponent -> hiding) &&
			     p -> aim_move != NULL && p -> aiming == 0 &&
			     GET_ATTACK_DICE(p) + GET_BONUS_ATTACK(p) > -3);
  p -> enabled_moves[RESTRAIN] = false;
  p -> enabled_moves[THREATEN] =
    (!(p -> restrained) &&
     p -> afraid < 2 &&
     p -> infatuated < 2 &&
     GET_THREATEN_DICE(p) + GET_SOCIAL_BONUS(p, oponent) > 0 &&
     p -> threaten_move != NULL &&
     ((oponent -> afraid && oponent -> resisted_threat[0] == false) ||
     (p -> initiative > oponent-> initiative &&
     p -> penalty[p -> damage] >
     oponent -> penalty[oponent -> damage] &&
     oponent -> resisted_threat[-(oponent -> penalty[oponent -> damage])/2] ==
      false)));
  p -> enabled_moves[THROW_POISON] = (!(p -> restrained) &&
				      !(p -> grappled &&
					oponent -> grapple_move == glare) &&
				      p -> infatuated < 2 &&
				      p -> friendly < 2 &&
				      p -> afraid < 2 &&
				      !(oponent -> hiding) &&
				      GET_ATTACK_DICE(p) +
					 GET_BONUS_ATTACK(p) > 0 &&
				      p -> poison_move != NULL &&
				      p -> poison_remaining > 0 &&
				      oponent -> type1 != POISON &&
				      oponent -> type2 != POISON &&
				      p -> initiative > 4);
  p -> enabled_moves[BURN] = (!(p -> restrained) &&
			      !(p -> grappled &&
				oponent -> grapple_move == glare) &&
			      p -> infatuated < 2 &&
			      p -> friendly < 2 &&
			      p -> afraid < 2 &&
			      !(oponent -> hiding) &&
			      GET_ATTACK_DICE(p) +
			      GET_BONUS_ATTACK(p) > 0 &&
			      p -> burn_move != NULL &&
			      p -> poison_remaining > 0 &&
			      oponent -> type1 != FIRE &&
			      oponent -> type2 != FIRE &&
			      p -> initiative > 4);
  p -> enabled_moves[END_BURN] = (p -> burned);
  p -> enabled_moves[PARALYZE] = (!(p -> restrained) &&
				  !(p -> grappled &&
				    oponent -> grapple_move == glare) &&
				  p -> infatuated < 2 &&
				  p -> friendly < 2 &&
				  p -> afraid < 2 &&
				  !(oponent -> hiding) &&
				  GET_ATTACK_DICE(p) +
				  GET_BONUS_ATTACK(p) > 0 &&
				  p -> paralysis_move != NULL &&
				  p -> poison_remaining > 0 &&
				  oponent -> type1 != ELECTRIC &&
				  oponent -> type2 != ELECTRIC &&
				  !(p -> paralysis_move == stun_spore &&
				    (oponent -> type1 == GRASS ||
				     oponent -> type2 == GRASS)) &&
				  p -> initiative > 4);
  p -> enabled_moves[MAKE_SLEEP] = (!(p -> restrained) &&
				    !(p -> grappled &&
				      oponent -> grapple_move == glare) &&
				    p -> angry < 2 &&
				    GET_MAGIC_DICE(p) > 0 &&
				    (p -> making_sleep ||
				    (p -> sleep_move != NULL &&
				    p -> willpower >= 2 &&
				     p -> sleep_move != NULL &&
				     (p -> sleep_move != sleep_powder ||
				      (p -> sleep_move == sleep_powder &&
				       oponent -> type1 != GRASS &&
				       oponent -> type2 != GRASS)))));
  p -> enabled_moves[SMOKE] = (!(p -> restrained) &&
			       !(p -> grappled &&
				 oponent -> grapple_move == glare) &&
			       !(p -> paralyzed && p -> initiative <= 0) &&
			       p -> infatuated < 2 &&
			       p -> friendly < 2 &&
			       !(oponent -> hiding) &&
			       p -> angry < 2 &&
			       !(p -> prone) &&
			       GET_BONUS_MOVE(p) + GET_ATTACK_DICE(p) > 0 &&
			       p -> smoke_move != NULL && !(p -> grappled) &&
			       oponent -> other_senses == false &&
			       p -> distance == 0 && oponent -> distance == 0);
  p -> enabled_moves[CLEAN_EYES] = (!(p -> restrained) && p -> blinded &&
				    !(p -> grappled &&
				      oponent -> grapple_move == glare));
  p -> enabled_moves[FULL_DEFENSE] = (!(p -> restrained) &&
				      !(p -> grappled &&
					oponent -> grapple_move == glare) &&
				      p -> angry < 2 &&
				      p -> full_defense_move != NULL &&
				      p -> initiative > 0);
  p -> enabled_moves[DISENGAGE] = (!(p -> restrained) &&
				   !(p -> paralyzed && p -> initiative <= 0) &&
				   !(p -> grappled &&
				     oponent -> grapple_move == glare) &&
				   p -> angry < 2 &&
				   GET_BONUS_MOVE(p) + GET_ATTACK_DICE(p) > 0 &&
				   p -> distance == 0 &&
				   oponent -> distance == 0 &&
				   !(p -> grappled) &&
				   p -> can_teleport);
  p -> enabled_moves[ATT_DISENGAGE] = (!(p -> restrained) &&
				       !(p -> paralyzed &&
					 p -> initiative <= 0) &&
				       !(p -> grappled &&
					oponent -> grapple_move == glare) &&
				       p -> infatuated < 2 &&
				       p -> friendly < 2 &&
				       !(oponent -> hiding) &&
				       !(p -> prone) &&
				       p -> distance == 0 &&
				       oponent -> distance == 0 &&
				       !(p -> grappled) &&
				       p -> att_disengage_move != NULL &&
				       p -> initiative > 0 &&
				       (GET_ATTACK_DICE(p) +
				       GET_BONUS_ATTACK(p) > 3 ||
					GET_BONUS_MOVE(p) +
					GET_ATTACK_DICE(p) > 3));
  p -> enabled_moves[AIM_ATT] = (!(p -> restrained) &&
				 !(p -> grappled &&
				   oponent -> grapple_move == glare) &&
				 p -> infatuated < 2 &&
				 p -> afraid < 2 &&
				 p -> friendly < 2 &&
				 !(oponent -> hiding) &&
				 GET_ATTACK_DICE(p) +
				 GET_BONUS_ATTACK(p) > -3 &&
				 p -> aim_att_move != NULL &&
				 p -> initiative > 0);
  p -> enabled_moves[HIDE_ATT] = (!(p -> restrained) &&
				  !(p -> paralyzed && p -> initiative <= 0) &&
				  !(p -> grappled &&
				    oponent -> grapple_move == glare) &&
				  p -> infatuated < 2 &&
				  p -> afraid < 3 &&
				  p -> friendly < 2 &&
				  !(oponent -> hiding) &&
				  GET_ATTACK_DICE(p)  > 0 &&
				 p -> hide_att_move != NULL &&
				 p -> initiative > 0);
  p -> enabled_moves[TAKE_COVER] = (!(p -> restrained) &&
				    !(p -> paralyzed && p -> initiative <= 0) &&
				    !(p -> grappled &&
				      oponent -> grapple_move == glare) &&
				    p -> angry < 2 &&
				    !(p -> prone) &&
				    p -> take_cover_move != NULL &&
				    p -> covered == 0 &&
				    !(p -> grappled) &&
				    GET_ATTACK_DICE(p) + GET_BONUS_MOVE(p) > 0);
  p -> enabled_moves[DIF_TERRAIN] = (!(p -> restrained) &&
				     !(p -> grappled &&
					oponent -> grapple_move == glare) &&
				     p -> infatuated < 2 &&
				     p -> angry < 2 &&
				     p -> dif_terrain_move != NULL &&
				     oponent -> difficult_terrain == false);
  p -> enabled_moves[GRAPPLE] = (!(p -> restrained) &&
				 !(p -> grappled &&
				   oponent -> grapple_move == glare) &&
				 p -> infatuated < 2 &&
				 p -> afraid < 2 &&
				 !(oponent -> hiding) &&
				 p -> grapple_move != NULL &&
				 p -> distance == 0 &&
				 oponent -> distance == 0 &&
				 p -> initiative > 3 &&
				 !(p -> grappled) &&
				 p -> size >= oponent -> size &&
				 oponent -> shape != MULTIPLE_BODIES &&
				 GET_ATTACK_DICE(p) + GET_BONUS_ATTACK(p) > 0);
  p -> enabled_moves[SLAM] = (!(p -> restrained) &&
			      !(p -> grappled &&
				oponent -> grapple_move == glare) &&
			      p -> infatuated < 2 &&
			      p -> afraid < 2 &&
			      p -> friendly < 2 &&
			      !(oponent -> hiding) &&
			      p -> slam_move != NULL &&
			      p -> grapple_move == NULL &&
			      p -> distance == 0 &&
			      oponent -> distance == 0 &&
			      p -> initiative > 3 &&
			      !(p -> grappled) &&
			      oponent -> shape != MULTIPLE_BODIES &&
			      p -> size >= oponent -> size &&
			      GET_ATTACK_DICE(p) + GET_BONUS_ATTACK(p) > 0);
  p -> enabled_moves[GET_UP] = (p -> prone == true &&
				!(p -> broken_leg) &&
				!(p -> paralyzed && p -> initiative <= 0) &&
				!(p -> grappled &&
				  oponent -> grapple_move == glare) &&
				GET_ATTACK_DICE(p) + GET_BONUS_MOVE(p) > 0);
  p -> enabled_moves[AMBUSH] = (p -> first_turn &&
				!(p -> grappled &&
				  oponent -> grapple_move == glare) &&
				p -> initiative > oponent -> initiative &&
				p -> ambush_move != NULL && a1 > a2 &&
				compute_weakness(p -> ambush_type, oponent) < 3);
  p -> enabled_moves[SCARE] = (p -> scare_move != NULL &&
			       !(p -> grappled &&
				 oponent -> grapple_move == glare) &&
			       GET_MANIPULATION_DICE(p) > 0 &&
			       p -> infatuated < 2 &&
			       p -> afraid < 2 &&
			       !(p -> restrained));
  p -> enabled_moves[ANGER] = (p -> anger_move != NULL &&
			       !(p -> grappled &&
				 oponent -> grapple_move == glare) &&
			       GET_THREATEN_DICE(p) > 0 &&
			       p -> infatuated < 2 &&
			       p -> afraid < 2 &&
			       !(p -> restrained));
  p -> enabled_moves[FRIENDSHIP] = (p -> friendship_move != NULL &&
				    ((p -> friendship_move == fake_tears &&
				     GET_MANIPULATION_DICE(p) > 0) ||
				     (p -> friendship_move != fake_tears &&
				      GET_CHARISMA_DICE(p) > 0)) &&
				    !(p -> grappled &&
				      oponent -> grapple_move == glare) &&
				    oponent -> infatuated == 0 &&
				    p -> afraid < 2 &&
				    p -> angry < 2 &&
				    oponent -> friendly == 0 &&
				    !(p -> restrained));
  p -> enabled_moves[FLIRT] = (p -> flirt_move != NULL &&
			       !(p -> grappled &&
				 oponent -> grapple_move == glare) &&
			       oponent -> infatuated == 0 &&
			       GET_CHARISMA_DICE(p) > 0 &&
			       p -> afraid < 2 &&
			       p -> angry < 2 &&
			       !(p -> restrained) &&
			       !(oponent -> genderless) &&
			       !(p -> genderless));
    p -> enabled_moves[SEDUCE] =
    (!(p -> restrained) &&
     !(p -> grappled &&
       oponent -> grapple_move == glare) &&
     oponent -> infatuated > 0 &&
     p -> afraid < 2 &&
     GET_CHARISMA_DICE(p) + GET_SOCIAL_BONUS(p, oponent) > 0 &&
     p -> seduce_move != NULL);
    p -> enabled_moves[TRANSFORM] = (p -> can_transform &&
				     !(p -> restrained));
}

void print_vitality(struct pokemon *p){
  int i;
  for(i = 0; i < p -> vitality; i ++){
    if(p -> damage > i)
      printf("[X]");
    else
      printf("[ ]");
  }
  printf("\n");
  printf("            ");
  for(i = 1; i <= p -> vitality; i ++){
    if(p -> penalty[i] == 0)
      printf(" 0 ");
    else
      printf("%d ", p -> penalty[i]);
  }  
}


void print_pokemon(struct pokemon *p, struct pokemon *oponent){
  int i;
  check_valid_moves(p, oponent);
  check_valid_moves(oponent, p);
  printf("------------------------------------------------------------------\n");
  printf("%s: %s\n", p -> name, p-> desc);
  if(p -> size == TINY)
    printf("(Corpo Pequeno)");
  else if(p -> size == LEGENDARY)
    printf("(Tamanho Lendário)");
  if(p -> poisoned)
    printf("(Envenenado)");
  if(p -> burned)
    printf("(Pegando Fogo)");
  if(p -> paralyzed)
    printf("(Paralisado)");
  if(p -> sleepy)
    printf("(Com Sono)");
  if(p -> blinded)
    printf("(Cego)");
  if(p -> crippling_penalty < 0 && p -> crippling_penalty > -5)
    printf("(Membro Deslocado)");
  if(p -> crippling_penalty == -5)
    printf("(Membro Quebrado)");
  if(p -> distance || oponent -> distance)
    printf("(Distante)");
  if(p -> difficult_terrain)
    printf("(Terreno Difícil)");
  if(p -> grappled && oponent -> grapple_move != glare)
    printf("(Preso)");
  if(p -> grappled && oponent -> grapple_move == glare)
    printf("(Enfeitiçado pelo Olhar)");
  if(p -> prone)
    printf("(Derrubado no Chão)");
  if(p -> control_turns > 0)
    printf("(Prendendo Oponente)");
  if(p -> afraid > 0)
    printf("(Assustado)");
  if(p -> friendly > 0)
    printf("(Com Dó)");
  if(p -> infatuated > 0)
    printf("(Apaixonado)");
  if(p -> angry > 0)
    printf("(Furioso)");
  if(p -> covered == 1)
    printf("(Parte do Corpo Coberto por %s)", p -> take_cover_move);
  else if(p -> covered == 2)
    printf("(Maioria do Corpo Coberto por %s)", p -> take_cover_move);
  else if(p -> covered == 3)
    printf("(Corpo Totalmente Coberto por %s)", p -> take_cover_move);
  printf("\n");
  printf("FORÇA   : [");
  for(i = 0; i < 5; i ++) if(i < p -> strength) printf("X"); else printf(" ");
  printf("] ");
  printf("CARISMA:     [");
  for(i = 0; i < 5; i ++) if(i < p -> charisma) printf("X"); else printf(" ");
  printf("] ");
  printf("PERCEPÇÃO:    [");
  for(i = 0; i < 5; i ++) if(i < p -> perception) printf("X"); else printf(" ");
  printf("] \n");
  printf("DESTREZA: [");
  for(i = 0; i < 5; i ++) if(i < p -> dexterity) printf("X"); else printf(" ");
  printf("] ");
  printf("MANIPULAÇÃO: [");
  for(i = 0; i < 5; i ++) if(i < p -> manipulation) printf("X"); else printf(" ");
  printf("] ");
  printf("INTELIGÊNCIA: [");
  for(i = 0; i < 5; i ++) if(i < p -> inteligence) printf("X"); else printf(" ");
  printf("] \n");
  printf("VIGOR:    [");
  for(i = 0; i < 5; i ++) if(i < p -> stamina) printf("X"); else printf(" ");
  printf("] ");
  printf("APARÊNCIA:   [");
  for(i = 0; i < 5; i ++) if(i < p -> appearance) printf("X"); else printf(" ");
  printf("] ");
  printf("RACIOCÍNIO:   [");
  for(i = 0; i < 5; i ++) if(i < p -> wits) printf("X"); else printf(" ");
  printf("] \n");
  printf("Habilidade: %d\n", p -> essence);
  printf("Vitalidade: ");
  print_vitality(p);
  printf("\n");
  printf("Força de Vontade: ");
  for(i = 0; i < 10; i ++){
    if(p -> willpower > i)
      printf("[X]");
    else
      printf("[ ]");
  }
  printf("\n\n");
  if(p -> initiative > 0){
    i = (p -> initiative - 1) / 2;
    if(i > 25) i = 25;
    while(p -> description[i] == NULL)
      i --;
    printf("Ataque Decisivo: %s (%s) (%s Distância) (%d dados)\n",
	   p -> description[i],
	   (i < 0 || p -> type[i] < 0)?("Sem Tipo"):(string_type[p -> type[i]]),
	   (i < 0 || p -> ranged[i] == false)?("Curta"):("Longa"),
	   (GET_ATTACK_DICE(p) + GET_BONUS_ATTACK(p) > 0)?
	   (GET_ATTACK_DICE(p) + GET_BONUS_ATTACK(p)):(0));
  }
  else
    printf("Ataque Decisivo: -\n");
  printf("Ataque Genérico: %d dados    Dano Físico: %d    Dano Especial: %d\n",
	 (GET_WITHERING_DICE(p) + GET_BONUS_ATTACK(p) > 0)?
	 (GET_WITHERING_DICE(p) + GET_BONUS_ATTACK(p)):(0),
	 p -> strength + p -> physical_damage,
	 p -> inteligence + p -> special_damage);
  printf("Defesa: %.1f ", GET_DEFENSE(p) + GET_BONUS_DEFENSE(p, oponent));
  if(GET_BONUS(p)/2.0 + GET_BONUS_DEFENSE(p, oponent) < 0)
    printf("(%.1f)", GET_BONUS(p)/2.0 + GET_BONUS_DEFENSE(p, oponent));
  else if(GET_BONUS(p)/2.0 + GET_BONUS_DEFENSE(p, oponent) > 0)
    printf("(+%.1f)", GET_BONUS(p)/2.0 + GET_BONUS_DEFENSE(p, oponent));
  printf("   Absorção Física: %d     Absorção Especial: %d",
	 p -> stamina + p -> physical_soak,
	 p -> wits + p -> special_soak);
  printf("\n");
  printf("Iniciativa: %d\n", p -> initiative);
  printf("------------------------------------------------------------------\n");
}




struct pokemon *new_bulbasaur(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Bulbasaur");
  strcpy(p -> desc, "Pokémon Semente");
  p -> shape = QUADRUPED;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> damage = 0;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(SEED_BOMB);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder;
  p -> sleep_move = sleep_powder;
  p -> aim_att_move = solar_beam; p -> aim_att_type = GRASS;
  p -> aim_att_ranged = true;
  p -> friendship_move = growl;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_ivysaur(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Ivysaur");
  strcpy(p -> desc, "Pokémon Semente");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> damage = 0;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(SEED_BOMB);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> sleep_move = sleep_powder;
  p -> aim_att_move = solar_beam; p -> aim_att_type = GRASS;
  p -> aim_att_ranged = true;
  p -> friendship_move = growl;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_venusaur(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Venusaur");
  strcpy(p -> desc, "Pokémon Semente");
  p -> shape = QUADRUPED;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> damage = 0;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 3; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(SEED_BOMB);
  DESCRIPTION(PETAL_BLIZZARD);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(PETAL_DANCE);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> sleep_move = sleep_powder;
  p -> aim_att_move = solar_beam; p -> aim_att_type = GRASS;
  p -> aim_att_ranged = true;
  p -> friendship_move = growl;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_charmander(int team){
  struct pokemon *p = new_base_pokemon(team);
  p -> shape = BIPEDAL;
  strcpy(p -> name, "Charmander");
  strcpy(p -> desc, "Pokémon Lagarto");
  p -> vitality = 4; //39
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> damage = 0;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(EMBER);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(SLASH);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  p -> friendship_move = growl;
  return p;
}

struct pokemon *new_charmeleon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Charmeleon");
  strcpy(p -> desc, "Pokémon Chama");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //58
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> damage = 0;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(EMBER);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(SLASH);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  p -> friendship_move = growl;
  return p;
}

struct pokemon *new_charizard(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Charizard");
  strcpy(p -> desc, "Pokémon Chama");
  p -> shape = BIPEDAL;
  p -> vitality = 8; //78
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4; p -> penalty[8] = -4;
  p -> damage = 0;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = FIRE; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(EMBER);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(SLASH);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(DRAGON_CLAW);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(HEAT_WAVE);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  p -> friendship_move = growl;
  return p;
}

struct pokemon *new_squirtle(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Squirtle");
  strcpy(p -> desc, "Pokémon Tartaruguinha");
  p -> shape = BIPEDAL;
  p -> vitality = 5; //44
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(BITE);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> aim_att_move = skull_bash; p -> aim_att_type = NORMAL;
  p -> aim_att_ranged = false;
  p -> friendship_move = tail_whip;
  p -> physical_soak = 2; // Iron Defense
  return p;
}

struct pokemon *new_wartortle(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Wartortle");
  strcpy(p -> desc, "Pokémon Tartaruga");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //59
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(BITE);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> aim_att_move = skull_bash; p -> aim_att_type = NORMAL;
  p -> aim_att_ranged = false;
  p -> friendship_move = tail_whip;
  p -> physical_soak = 2; // Iron Defense
  return p;
}

struct pokemon *new_blastoise(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Blastoise");
  strcpy(p -> desc, "Pokémon Fruto do Mar");
  p -> shape = BIPEDAL;
  p -> vitality = 8; //79
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4; p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 5; // Starter
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(BITE);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(FLASH_CANNON);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> aim_att_move = skull_bash; p -> aim_att_type = NORMAL;
  p -> aim_att_ranged = false;
  p -> friendship_move = tail_whip;
  p -> physical_soak = 2; // Iron Defense
  return p;
}

struct pokemon *new_caterpie(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Caterpie");
  strcpy(p -> desc, "Pokémon Verme");
  p -> shape = INSECTOID;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -3; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 0; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 0; // 55% Viridian Forest
  p -> willpower = 0;
  p -> type1 = BUG; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(BUG_BITE);
  END_DESCRIPTION();
  return p;
}

struct pokemon *new_metapod(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Metapod");
  strcpy(p -> desc, "Pokémon Casulo");
  p -> shape = SERPENTINE;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 0; p -> dexterity = 0; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 1; // 40% Viridian Forest
  p -> willpower = 5;
  p -> type1 = BUG; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  END_DESCRIPTION();
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_butterfree(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Butterfree");
  strcpy(p -> desc, "Pokémon Borboleta");
  p -> shape = FLYING_BUG;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 3;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = BUG; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
    DESCRIPTION(TACKLE);
  DESCRIPTION(GUST);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(BUG_BITE);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(BUG_BUZZ);
  END_DESCRIPTION();
  p -> poison_move = poison_powder;
  p -> paralysis_move = stun_spore;
  p -> sleep_move = sleep_powder;
  p -> other_senses = true;
  p -> slam_move = whirlwind;
  p -> slam_type = FLYING;
  p -> accuracy = 1;p -> special_damage = 1; p -> special_soak = 1;// Quiver Dance
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_weedle(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Weedle");
  strcpy(p -> desc, "Pokémon Inseto Cabeludo");
  p -> shape = INSECTOID;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 1; // 45% ViridianForest 
  p -> willpower = 0;
  p -> type1 = BUG; p -> type2 = POISON;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(BUG_BITE);
  END_DESCRIPTION();
  return p;
}

struct pokemon *new_kakuna(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kakuna");
  strcpy(p -> desc, "Pokémon Casulo");
  p -> shape = SERPENTINE;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 0; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 1; //40%, Floresta Vir.
  p -> willpower = 5;
  p -> type1 = BUG; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  END_DESCRIPTION();
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_beedrill(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Beedrill");
  strcpy(p -> desc, "Pokémon Abelha Venenosa");
  p -> shape = FLYING_BUG;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = BUG; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> accuracy = 2; // Agility
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(PIN_MISSILE);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(FELL_STINGER);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(BUG_BITE);
  DESCRIPTION(VENOSHOCK);
  DESCRIPTION(POISON_JAB);
  END_DESCRIPTION();
  p -> physical_soak = 1; //Harden
  p -> accuracy = 2; //Agility
  p -> dif_terrain_move = toxic_spikes;
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_pidgey(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Pidgey");
  strcpy(p -> desc, "Pokémon Passarinho");
  p -> shape = BIRD;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 70%, rota 1
  p -> willpower = 0;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = TINY;
  p -> team = team;
  p -> accuracy = 2; // Agility
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(GUST);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(HURRICANE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> slam_move = whirlwind;
  p -> slam_type = FLYING;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_pidgeotto(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Pidgeotto");
  strcpy(p -> desc, "Pokémon Pássaro");
  p -> shape = BIRD;
  p -> vitality = 7; //
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 15% rota 13
  p -> willpower = 5;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(GUST);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(HURRICANE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> slam_move = whirlwind;
  p -> slam_type = FLYING;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_pidgeot(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Pidgeot");
  strcpy(p -> desc, "Pokémon Pássaro");
  p -> shape = BIRD;
  p -> vitality = 8; //83
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(GUST);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(HURRICANE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> slam_move = whirlwind;
  p -> slam_type = FLYING;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_rattata(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Rattata");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = QUADRUPED;
  p -> vitality = 3; //83
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 1; p -> perception = 1; // 50% rota 1
  p -> willpower = 0;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(BITE);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> friendship_move = tail_whip;
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_raticate(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Raticate");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 1; //40% Pm Mansion
  p -> willpower = 5;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(BITE);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> friendship_move = tail_whip;
  p -> physical_damage = 2; // Swords Dance
  p -> scare_move = scary_face;
    p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_spearow(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Spearow");
  strcpy(p -> desc, "Pokémon Passarinho");
  p -> shape = BIRD;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 0; //55% rota 3
  p -> willpower = 0;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(PECK);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(DRILL_PECK);
  DESCRIPTION(TAKE_DOWN);
  END_DESCRIPTION();
  p -> scare_move  = leer;
  p -> friendship_move = growl;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_fearow(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Fearow");
  strcpy(p -> desc, "Pokémon Bicudo");
  p -> shape = BIRD;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; //25% rota 23
  p -> willpower = 6;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(PECK);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(PLUCK);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(DRILL_RUN);
  DESCRIPTION(DRILL_PECK);
  DESCRIPTION(TAKE_DOWN);
  END_DESCRIPTION();
  p -> scare_move  = leer;
  p -> friendship_move = growl;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_ekans(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Ekans");
  strcpy(p -> desc, "Pokémon Cobra");
  p -> shape = SERPENTINE;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; //40% rota 11
  p -> willpower = 0;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(ACID);
  DESCRIPTION(ACID_SPRAY);
  DESCRIPTION(BITE);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(BELCH);
  DESCRIPTION(GUNK_SHOT);
  END_DESCRIPTION();
  p -> grapple_move = glare;
  p -> grapple_type = NONE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move  = leer;
  p -> poison_move = toxic;
  p -> physical_damage = 1; p -> physical_soak = 1; // Coil
  return p;
}

struct pokemon *new_arbok(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Arbok");
  strcpy(p -> desc, "Pokémon Naja");
  p -> shape = SERPENTINE;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 3; // 10% Cerulean Cave
  p -> willpower = 6;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(ACID);
  DESCRIPTION(ACID_SPRAY);
  DESCRIPTION(BITE);
  DESCRIPTION(ICE_FANG);
  DESCRIPTION(THUNDER_FANG);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(BELCH);
  DESCRIPTION(GUNK_SHOT);
  END_DESCRIPTION();
  p -> grapple_move = glare;
  p -> grapple_type = NONE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move  = leer;
  p -> poison_move = toxic;
  p -> physical_damage = 1; p -> physical_soak = 1; // Coil
  return p;
}

struct pokemon *new_pikachu(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Pikachu");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = QUADRUPED;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; //25% Power Plant
  p -> willpower = 2;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(NUZZLE);
  DESCRIPTION(FEINT);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(SPARK);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(THUNDERBOLT);
  DESCRIPTION(THUNDER);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> friendship3 = play_nice;
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> accuracy = 2; // Agility
  p -> special_damage = 2; // Nasty Plot
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_raichu(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Raichu");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 3; //10% cerulean cave
  p -> willpower = 7;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(NUZZLE);
  DESCRIPTION(FEINT);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(SPARK);
  DESCRIPTION(THUNDER_PUNCH);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(THUNDERBOLT);
  DESCRIPTION(THUNDER);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> friendship_move = growl;
  p -> friendship2 = play_nice;
  p -> friendship3 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> accuracy = 2; // Agility
  p -> special_damage = 2; // Nasty Plot
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_sandshrew(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Sandshrew");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = BIPEDAL;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 1; //40% Rota 11
  p -> willpower = 0;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(SWIFT);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(SLASH);
  DESCRIPTION(EARTHQUAKE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> hide_att_move = dig; p -> aim_att_type = GROUND;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 2; // Swords Dance
  p -> physical_soak = 1; // Defense Curl
  p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_sandslash(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Sandslash");
  strcpy(p -> desc, "Pokémon Rato");
  p -> shape = BIPEDAL;
  p -> vitality = 8; //75
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 3; // 10% Cerulean Cave
  p -> willpower = 6;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(SWIFT);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION("Garra Esmagadora", NORMAL, false);
  DESCRIPTION(SLASH);
  DESCRIPTION(EARTHQUAKE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> grapple_move = sand_tomb;
  p -> grapple_type = GROUND;
  p ->  grapple_category = PHYSICAL;
  p -> hide_att_move = dig; p -> aim_att_type = GROUND;
  p -> physical_damage = 2; // Swords Dance
  p -> physical_soak = 1; // Defense Curl
  p -> poison_remaining = 2;
  return p;
}

struct pokemon *new_nidoran_f(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidoran♀");
  strcpy(p -> desc, "Pokémon do Ferrão Venenoso");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; //40% Rota 22
  p -> willpower = 0;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(SCRATCH);
  DESCRIPTION("Morder", DARK, false);
  DESCRIPTION("Mastigar", DARK, false);
  DESCRIPTION(EARTH_POWER);
  END_DESCRIPTION();
  p -> dif_terrain_move = toxic_spikes;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_nidorina(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidorina");
  strcpy(p -> desc, "Pokémon do Ferrão Venenoso");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Rota 23
  p -> willpower = 5;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(SCRATCH);
  DESCRIPTION("Morder", DARK, false);
  DESCRIPTION("Mastigar", DARK, false);
  DESCRIPTION(EARTH_POWER);
  END_DESCRIPTION();
  p -> dif_terrain_move = toxic_spikes;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_nidoqueen(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidoqueen");
  strcpy(p -> desc, "Pokémon Furadora");
  p -> shape = BIPEDAL;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = POISON; p -> type2 = GROUND;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(SCRATCH);
  DESCRIPTION("Morder", DARK, false);
  DESCRIPTION("Mastigar", DARK, false);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(SUPERPOWER);
  END_DESCRIPTION();
  p -> other_senses = true;
  p -> dif_terrain_move = toxic_spikes;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_nidoran_m(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidoran♂");
  strcpy(p -> desc, "Pokémon do Ferrão Venenoso");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //46
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; //40% ...
  p -> willpower = 0;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(PECK);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(EARTH_POWER);
  END_DESCRIPTION();
  p -> dif_terrain_move = toxic_spikes;
  p -> scare_move  = leer;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_nidorino(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidorino");
  strcpy(p -> desc, "Pokémon do Ferrão Venenoso");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //61
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 35% Rota 23
  p -> willpower = 5;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(PECK);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(EARTH_POWER);
  END_DESCRIPTION();
  p -> dif_terrain_move = toxic_spikes;
  p -> scare_move  = leer;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_nidoking(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Nidoking");
  strcpy(p -> desc, "Pokémon Furador");
  p -> shape = BIPEDAL;
  p -> vitality = 8; //81
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = POISON; p -> type2 = GROUND;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(PECK);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(MEGAHORN);
  END_DESCRIPTION();
  p -> other_senses = true;
  p -> dif_terrain_move = toxic_spikes;
  p -> scare_move  = leer;
  p -> poison_move = toxic; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_clefairy(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Clefairy");
  strcpy(p -> desc, "Pokémon Fada");
  p -> shape = BIPEDAL;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 5; p -> manipulation = 1; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 3; //10% Mt. Lua
  p -> willpower = 4;
  p -> type1 = FAIRY; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(STORED_POWER);
  DESCRIPTION(POUND);
  DESCRIPTION(DISARMING_VOICE);
  DESCRIPTION(METEOR_MASH);
  DESCRIPTION(MOONBLAST);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> physical_soak = 1; p -> special_soak = 1; // Cosmic Power
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_clefable(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Clefable");
  strcpy(p -> desc, "Pokémon Fada");
  p -> shape = BIPEDAL;
  p -> vitality = 9; //95
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 5; p -> manipulation = 1; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = FAIRY; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(STORED_POWER);
  DESCRIPTION(POUND);
  DESCRIPTION(DISARMING_VOICE);
  DESCRIPTION(METEOR_MASH);
  DESCRIPTION(MOONBLAST);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> physical_soak = 1; p -> special_soak = 1; // Cosmic Power
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_vulpix(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Vulpix");
  strcpy(p -> desc, "Pokémon Raposa");
  p -> shape = QUADRUPED;
  p -> vitality = 4; //38
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 2; // 25% Mansão Pkm
  p -> willpower = 2;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(EMBER);
  DESCRIPTION(INCINERATE);
  DESCRIPTION(EXTRASENSORY);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FIRE_BLAST);
  END_DESCRIPTION();
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> friendship_move = tail_whip;
  p -> burn_move = will_o_wisp;
  return p;
}

struct pokemon *new_ninetales(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Ninetales");
  strcpy(p -> desc, "Pokémon Raposa");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //73
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 2;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(EMBER);
  DESCRIPTION(INCINERATE);
  DESCRIPTION(EXTRASENSORY);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FIRE_BLAST);
  END_DESCRIPTION();
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> friendship_move = tail_whip;
  p -> special_damage = 2; // Nasty Plot
  p -> burn_move = will_o_wisp;
  return p;
}

struct pokemon *new_jigglypuff(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Jigglypuff");
  strcpy(p -> desc, "Pokémon Balão");
  p -> shape = HUMANOID;
  p -> vitality = 12; //115
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -2; p -> penalty[10] = -4; p -> penalty[11] = -4;
  p -> penalty[12] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 0; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 1; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 0; p -> perception = 3; //10% Kanto 3
  p -> willpower = 3;
  p -> type1 = NORMAL; p -> type2 = FAIRY;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DISARMING_VOICE);
  DESCRIPTION(POUND);
  DESCRIPTION(ECHOED_VOICE);
  DESCRIPTION(COVET);
  DESCRIPTION(ROUND);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(HYPER_VOICE);
  DESCRIPTION(PLAY_ROUGH);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> physical_soak = 1; // Defense Curl
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_wigglytuff(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Wigglytuff");
  strcpy(p -> desc, "Pokémon Balão");
  p -> shape = HUMANOID;
  p -> vitality = 14; //140
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -1; p -> penalty[7] = -1; p -> penalty[8] = -2;
  p -> penalty[9] = -2; p -> penalty[10] = -2; p -> penalty[11] = -2;
  p -> penalty[12] = -2; p -> penalty[13] = -4; p -> penalty[14] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 1; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 4; //5% Cerulean Cave
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = FAIRY;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DISARMING_VOICE);
  DESCRIPTION(POUND);
  DESCRIPTION(ECHOED_VOICE);
  DESCRIPTION(COVET);
  DESCRIPTION(ROUND);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(HYPER_VOICE);
  DESCRIPTION(PLAY_ROUGH);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> physical_soak = 1; // Defense Curl
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_zubat(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Zubat");
  strcpy(p -> desc, "Pokémon Morcego");
  p -> shape = BIRD;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 1; p -> perception = 0; //79% Mt Lua
  p -> willpower = 0;
  p -> type1 = POISON; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(ASTONISH);
  DESCRIPTION(POISON_FANG);
  DESCRIPTION(BITE);
  DESCRIPTION(AIR_CUTTER);
  DESCRIPTION(VENOSHOCK);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(LEECH_LIFE);
  END_DESCRIPTION();
  p -> other_senses = true;
  return p;
}

struct pokemon *new_golbat(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Golbat");
  strcpy(p -> desc, "Pokémon Morcego");
  p -> shape = BIRD;
  p -> vitality = 8; //75
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 1; //40% Cerulean Cave
  p -> willpower = 6;
  p -> type1 = POISON; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(ASTONISH);
  DESCRIPTION(POISON_FANG);
  DESCRIPTION(BITE);
  DESCRIPTION(AIR_CUTTER);
  DESCRIPTION(VENOSHOCK);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(LEECH_LIFE);
  END_DESCRIPTION();
  p -> other_senses = true;
  return p;
}

struct pokemon *new_oddish(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Oddish");
  strcpy(p -> desc, "Pokémon Erva");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 0; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 1; //40% Rota 12
  p -> willpower = 0;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(ACID);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(MOONBLAST);
  DESCRIPTION(PETAL_DANCE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder;
  p -> paralysis_move = stun_spore;
  p -> sleep_move = sleep_powder;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_gloom(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Gloom");
  strcpy(p -> desc, "Pokémon Erva");
  p -> shape = HUMANOID;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 4; //5% Rota 12
  p -> willpower = 5;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(ACID);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(MOONBLAST);
  DESCRIPTION(PETAL_DANCE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> paralysis_move = stun_spore;
  p -> sleep_move = sleep_powder;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_vileplume(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Vileplume");
  strcpy(p -> desc, "Pokémon Flor");
  p -> shape = HUMANOID;
  p -> vitality = 8; //75
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;   p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; //None
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(ACID);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(PETAL_BLIZZARD);
  DESCRIPTION(MOONBLAST);
  DESCRIPTION(PETAL_DANCE);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> paralysis_move = stun_spore;
  p -> sleep_move = sleep_powder;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_paras(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Paras");
  strcpy(p -> desc, "Pokémon Cogumelo");
  p -> shape = INSECTOID;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 15% Mt. Lua
  p -> willpower = 2;
  p -> type1 = BUG; p -> type2 = GRASS;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(SLASH);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(X_SCISSOR);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_parasect(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Parasect");
  strcpy(p -> desc, "Pokémon Cogumelo");
  p -> shape = INSECTOID;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 2; // 15% Safari Zone
  p -> willpower = 7;
  p -> type1 = BUG; p -> type2 = GRASS;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
    DESCRIPTION(ABSORB);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(SLASH);
  DESCRIPTION(CROSS_POISON);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(X_SCISSOR);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  return p;
}

struct pokemon *new_venonat(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Venonat");
  strcpy(p -> desc, "Pokémon Inseto");
  p -> shape = HUMANOID;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 20% Rota 12
  p -> willpower = 2;
  p -> type1 = BUG; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(POISON_FANG);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(STRUGGLE_BUG);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(LEECH_LIFE);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> sleep_move = sleep_powder;
  p -> other_senses = true; // Supersonic
  return p;
}

struct pokemon *new_venomoth(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Venomoth");
  strcpy(p -> desc, "Pokémon Mariposa Venenosa");
  p -> shape = FLYING_BUG;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 3; // 15% Cerulean Cave
  p -> willpower = 7;
  p -> type1 = BUG; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(GUST);
  DESCRIPTION(POISON_FANG);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(STRUGGLE_BUG);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(LEECH_LIFE);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(BUG_BUZZ);
  END_DESCRIPTION();
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> sleep_move = sleep_powder;
  p -> other_senses = true;
  p -> accuracy = 1;p -> special_damage = 1; p -> special_soak = 1;// Quiver Dance
  p -> slam_move = whirlwind;  p -> slam_type = FLYING;
  return p;
}

struct pokemon *new_digllet(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Digllet");
  strcpy(p -> desc, "Pokémon Toupeira");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 1; //10
  p -> penalty[0] = 0; p -> penalty[1] = -2;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 95% Caverna Digglet
  p -> willpower = 0;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(ASTONISH);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(SLASH);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(EARTHQUAKE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> hide_att_move = dig; p -> aim_att_type = GROUND;
  p -> friendship_move = growl;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_dugtrio(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dugtrio");
  strcpy(p -> desc, "Pokémon Toupeira");
  p -> shape = MULTIPLE_BODIES;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 4; // 5% Caverna Digglet
  p -> willpower = 8;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(ASTONISH);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(NIGHT_SLASH);
  DESCRIPTION(SLASH);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(TRI_ATTACK);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(EARTHQUAKE);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> grapple_move = sand_tomb;
  p -> grapple_type = GROUND;
  p ->  grapple_category = PHYSICAL;
  p -> hide_att_move = dig; p -> aim_att_type = GROUND;
  p -> friendship_move = growl;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_meowth(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Meowth");
  strcpy(p -> desc, "Pokémon Gato Arranhador");
  p -> shape = QUADRUPED;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 4; // 30% Rota 7
  p -> willpower = 0;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(FEINT);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(PAY_DAY);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(BITE);
  DESCRIPTION(SLASH);
  DESCRIPTION(PLAY_ROUGH);
  END_DESCRIPTION();
  p -> ambush_move = fake_out;
  p ->  ambush_type = NORMAL;
  p ->  ambush_attack = WITHERING_ATTACK;
  p -> anger_move = taunt;
  p -> friendship_move = growl;
  p -> special_damage = 2; // Nasty Plot
  return p;
}

struct pokemon *new_persian(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Persian");
  strcpy(p -> desc, "Pokémon Gato Elegante");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // Não encontrado
  p -> willpower = 6;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(FEINT);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(PAY_DAY);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(BITE);
  DESCRIPTION(SLASH);
  DESCRIPTION(POWER_GEM);
  DESCRIPTION(PLAY_ROUGH);
  END_DESCRIPTION();
  p -> ambush_move = fake_out;
  p ->  ambush_type = NORMAL;
  p ->  ambush_attack = WITHERING_ATTACK;
  p -> anger_move = taunt;
  p -> friendship_move = growl;
  p -> special_damage = 2; // Nasty Plot
  return p;
}

struct pokemon *new_psyduck(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Psyduck");
  strcpy(p -> desc, "Pokémon Pato");
  p -> shape = BIPEDAL;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 1; // 35% Seafom
  p -> willpower = 2;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> friendship_move = tail_whip;
  p -> special_soak = 2; // Amnesia
  return p;
}

struct pokemon *new_golduck(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Golduck");
  strcpy(p -> desc, "Pokémon Pato");
  p -> shape = BIPEDAL;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 4; // 5% Rota 6
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(AQUA_JET);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> friendship_move = tail_whip;
  p -> special_soak = 2; // Amnesia
  return p;
}

struct pokemon *new_mankey(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Mankey");
  strcpy(p -> desc, "Pokémon Macaco-Porco");
  p -> shape = BIPEDAL;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 30% Rota 7
  p -> willpower = 2;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(COVET);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(STOMPING_TANTRUM);
  DESCRIPTION(CROSS_CHOP);
  DESCRIPTION(THRASH);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(CLOSE_COMBAT);
  END_DESCRIPTION();
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> scare_move  = leer;
  p -> aim_att_move = skull_bash; p -> aim_att_type = NORMAL;
  p -> aim_att_ranged = false;
  return p;
}

struct pokemon *new_primeape(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Primeape");
  strcpy(p -> desc, "Pokémon Macaco-Porco");
  p -> shape = BIPEDAL;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 4; // 5% Rota 23
  p -> willpower = 7;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
    BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_SWIPES);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(COVET);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(STOMPING_TANTRUM);
  DESCRIPTION(CROSS_CHOP);
  DESCRIPTION(THRASH);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(CLOSE_COMBAT);
  END_DESCRIPTION();
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> scare_move  = leer;
  p -> aim_att_move = skull_bash; p -> aim_att_type = NORMAL;
  p -> aim_att_ranged = false;
  return p;
}

struct pokemon *new_growlithe(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Growlithe");
  strcpy(p -> desc, "Pokémon Filhote");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Mansão Pokemon
  p -> willpower = 2;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(EMBER);
  DESCRIPTION(BITE);
  DESCRIPTION(FLAME_WHEEL);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(PLAY_ROUGH);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> threaten_move = roar;
  p -> scare_move = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Howl
  return p;
}

struct pokemon *new_arcanine(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Arcanine");
  strcpy(p -> desc, "Pokémon Lendário");
  p -> shape = QUADRUPED;
  p -> vitality = 9; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 2;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Extreme Speed
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(EMBER);
  DESCRIPTION(BITE);
  DESCRIPTION(FLAME_WHEEL);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(EXTREME_SPEED);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(PLAY_ROUGH);
  DESCRIPTION(FLARE_BLITZ);
  DESCRIPTION(BURN_UP);
  END_DESCRIPTION();
  p -> threaten_move = roar;
  p -> scare_move = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Howl
  return p;
}

struct pokemon *new_poliwag(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Poliwag");
  strcpy(p -> desc, "Pokémon Girino");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 90% 22
  p -> willpower = 0;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(HYDRO_PUMP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_poliwhirl(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Poliwhirl");
  strcpy(p -> desc, "Pokémon Girino");
  p -> shape = HUMANOID;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 50% Rota 10
  p -> willpower = 5;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(HYDRO_PUMP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_poliwrath(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Poliwrath");
  strcpy(p -> desc, "Pokémon Girino");
  p -> shape = HUMANOID;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = FIGHTING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(SUBMISSION);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(EARTH_POWER);
  DESCRIPTION(DYNAMIC_PUNCH);
  DESCRIPTION(HYDRO_PUMP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> slam_move = circle_throw;
  p -> slam_type = FIGHTING;
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_abra(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Abra");
  strcpy(p -> desc, "Pokémon Psíquico");
  p -> shape = BIPEDAL;
  p -> vitality = 2; //25
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> essence = 1;
  p -> strength = 0; p -> dexterity = 2; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 3; p -> wits = 1; p -> perception = 2; // 25% rota 7
  p -> willpower = 2;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  END_DESCRIPTION();
  p -> can_teleport = true;
  return p;
}

struct pokemon *new_kadabra(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kadabra");
  strcpy(p -> desc, "Pokémon Psíquico");
  p -> shape = BIPEDAL;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 3; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 2; // 15% Cerulean cave
  p -> willpower = 6;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(PSYCHO_CUT);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(FUTURE_SIGHT);
  END_DESCRIPTION();
  p -> can_teleport = true;
  p -> mental_attack = true; // Psyshock
  p -> special_damage = 1; p -> special_soak = 1; // Calm Mind
  return p;
}

struct pokemon *new_alakazam(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Alakazam");
  strcpy(p -> desc, "Pokémon Psíquico");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 3;
  p -> strength = 1; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 4; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(PSYCHO_CUT);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(FUTURE_SIGHT);
  END_DESCRIPTION();
  p -> can_teleport = true;
  p -> mental_attack = true; // Psyshock
  p -> special_damage = 1; p -> special_soak = 1; // Calm Mind
  return p;
}

struct pokemon *new_machop(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Machop");
  strcpy(p -> desc, "Pokémon do Superpoder");
  p -> shape = BIPEDAL;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Victory Road
  p -> willpower = 2;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DUAL_CHOP);
  DESCRIPTION(REVENGE);
  DESCRIPTION(LOW_SWEEP);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(VITAL_THROW);
  DESCRIPTION(STRENGTH);
  DESCRIPTION(DYNAMIC_PUNCH);
  DESCRIPTION(CROSS_CHOP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> scare_move = scary_face;
  p -> physical_damage = 1; p -> physical_soak = 1; // Bulk up
  return p;
}

struct pokemon *new_machoke(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Machoke");
  strcpy(p -> desc, "Pokémon do Superpoder");
  p -> shape = HUMANOID;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 3; // 10% Victory Road
  p -> willpower = 6;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DUAL_CHOP);
  DESCRIPTION(REVENGE);
  DESCRIPTION(LOW_SWEEP);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(VITAL_THROW);
  DESCRIPTION(STRENGTH);
  DESCRIPTION(DYNAMIC_PUNCH);
  DESCRIPTION(CROSS_CHOP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> scare_move = scary_face;
  p -> physical_damage = 1; p -> physical_soak = 1; // Bulk up
  return p;
}

struct pokemon *new_machamp(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Machamp");
  strcpy(p -> desc, "Pokémon do Superpoder");
  p -> shape = HUMANOID;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 4; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DUAL_CHOP);
  DESCRIPTION(REVENGE);
  DESCRIPTION(LOW_SWEEP);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(VITAL_THROW);
  DESCRIPTION(STRENGTH);
  DESCRIPTION(DYNAMIC_PUNCH);
  DESCRIPTION(CROSS_CHOP);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> scare_move = scary_face;
  p -> physical_damage = 1; p -> physical_soak = 1; // Bulk up
  return p;
}


struct pokemon *new_bellsprout(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Bellsprout");
  strcpy(p -> desc, "Pokémon Flor");
  p -> shape = HUMANOID;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 0; p -> perception = 1; // 40% rota 12
  p -> willpower = 0;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ACID);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(POISON_JAB);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  p -> sleep_move = sleep_powder;
  return p;
}

struct pokemon *new_weepinbell(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Weepinbell");
  strcpy(p -> desc, "Pokémon Pega-Mosca");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 3; // 10% Cerulean Cave
  p -> willpower = 5;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ACID);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(POISON_JAB);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  p -> sleep_move = sleep_powder;
  return p;
}

struct pokemon *new_victreebel(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Victreebel");
  strcpy(p -> desc, "Pokémon Pega-Mosca");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ACID);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(RAZOR_LEAF);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(LEAF_TORNADO);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(LEAF_BLADE);
  DESCRIPTION(LEAF_STORM);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> poison_move = poison_powder; p -> poison_remaining = 1;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  p -> sleep_move = sleep_powder;
  return p;
}

struct pokemon *new_tentacool(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Tentacool");
  strcpy(p -> desc, "Pokémon Água-Viva");
  p -> shape = OCTOPUS;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 3; p -> perception = 0; // 100% Rota 19
  p -> willpower = 2;
  p -> type1 = WATER; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(ACID);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HEX);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(SURF);
  DESCRIPTION(SLUDGE_WAVE);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> other_senses = true;
  p -> physical_soak = 2; // Acid Armor
  p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_tentacruel(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Tentacruel");
  strcpy(p -> desc, "Pokémon Água-Viva");
  p -> shape = OCTOPUS;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 1; // 40% Rota 20
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POISON_STING);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(ACID);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HEX);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(SURF);
  DESCRIPTION(SLUDGE_WAVE);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> other_senses = true;
  p -> physical_soak = 2; // Acid Armor
  p -> poison_remaining = 1;
  return p;
}


struct pokemon *new_geodude(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Geodude");
  strcpy(p -> desc, "Pokémon Pedra");
  p -> shape = HEAD_WITH_ARMS;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 0; // 65% Victory Road
  p -> willpower = 0;
  p -> type1 = ROCK; p -> type2 = GROUND;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ROCK_THROW);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(EARTHQUAKE);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> dif_terrain_move = stealth_rock;
  p -> accuracy = 2; // Rock Polish
  p -> physical_soak = 1; // Defense Curl
  return p;
}

struct pokemon *new_graveler(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Graveler");
  strcpy(p -> desc, "Pokémon Pedra");
  p -> shape = HUMANOID;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 15% Victory Road
  p -> willpower = 5;
  p -> type1 = ROCK; p -> type2 = GROUND;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ROCK_THROW);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(EARTHQUAKE);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> dif_terrain_move = stealth_rock;
  p -> accuracy = 2; // Rock Polish
  p -> physical_soak = 1; // Defense Curl
  return p;
}

struct pokemon *new_golem(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Golem");
  strcpy(p -> desc, "Pokémon Megaton");
  p -> shape = HUMANOID;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 1; p -> stamina = 4;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = GROUND;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ROCK_THROW);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(EARTHQUAKE);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> dif_terrain_move = stealth_rock;
  p -> accuracy = 2; // Rock Polish
  p -> physical_soak = 1; // Defense Curl
  return p;
}

struct pokemon *new_ponyta(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Ponyta");
  strcpy(p -> desc, "Pokémon Cavalo de Fogo");
  p -> shape = QUADRUPED;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 1; // 40% Pokemon Mansion
  p -> willpower = 2;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(EMBER);
  DESCRIPTION(FLAME_CHARGE);
  DESCRIPTION(FLAME_WHEEL);
  DESCRIPTION(STOMP);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FIRE_BLAST);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_rapidash(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Rapidash");
  strcpy(p -> desc, "Pokémon Cavalo de Fogo");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 7;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(EMBER);
  DESCRIPTION(FLAME_CHARGE);
  DESCRIPTION(FLAME_WHEEL);
  DESCRIPTION(STOMP);
  DESCRIPTION(SMART_STRIKE);
  DESCRIPTION(POISON_JAB);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(INFERNO);
  DESCRIPTION(FIRE_BLAST);
  DESCRIPTION(MEGAHORN);
  DESCRIPTION(FLARE_BLITZ);
  END_DESCRIPTION();
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> accuracy = 2; // Agility
  p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_slowpoke(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Slowpoke");
  strcpy(p -> desc, "Pokémon Lerdo");
  p -> shape = QUADRUPED;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 95% Rota 12
  p -> willpower = 2;
  p -> type1 = WATER; p -> type2 = PSYCHIC;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(SURF);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> special_soak = 2; // Amnesia
  p -> sleep_move = yawn;
  return p;
}

struct pokemon *new_slowbro(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Slowbro");
  strcpy(p -> desc, "Pokémon com Caranguejo Hermitão");
  p -> shape = BIPEDAL;
  p -> vitality = 9; //95
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 2; // 25% Rota 23
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = PSYCHIC;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(SURF);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> physical_soak = 1; // Withdraw
  p -> special_soak = 2; // Amnesia
  p -> sleep_move = yawn;
  return p;
}


struct pokemon *new_magnemite(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Magnemite");
  strcpy(p -> desc, "Pokémon Ímã");
  p -> shape = HEAD_WITH_ARMS;
  p -> vitality = 2; //25
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 0; // 55% Rota 10
  p -> willpower = 2;
  p -> type1 = ELECTRIC; p -> type2 = STEEL;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(SPARK);
  DESCRIPTION(FLASH_CANNON);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(ZAP_CANNON);
  END_DESCRIPTION();
  p -> aim_move = lock_on;
  p -> other_senses = true;
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_magneton(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Magneton");
  strcpy(p -> desc, "Pokémon Ímã");
  p -> shape = MULTIPLE_BODIES;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 2; // 20% Power Plant
  p -> willpower = 7;
  p -> type1 = ELECTRIC; p -> type2 = STEEL;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(SPARK);
  DESCRIPTION(TRI_ATTACK);
  DESCRIPTION(FLASH_CANNON);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(ZAP_CANNON);
  END_DESCRIPTION();
  p -> aim_move = lock_on;
  p -> other_senses = true;
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_farfetchd(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Farfetch'd");
  strcpy(p -> desc, "Pokémon Pato Selvagem");
  p -> shape = BIRD;
  p -> vitality = 5; //52
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 4; // 5% Rota 12
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(PECK);
  DESCRIPTION(FURY_CUTTER);
  DESCRIPTION(FALSE_SWIPE);
  DESCRIPTION(CUT);
  DESCRIPTION(AERIAL_ACE);
  DESCRIPTION(AIR_CUTTER);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(SLASH);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(LEAF_BLADE);
  DESCRIPTION(BRAVE_BIRD);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 2; // Swords Dance
  return p;
}

struct pokemon *new_doduo(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Doduo");
  strcpy(p -> desc, "Pokémon Pássaros Gêmeos");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 55% Rota 17
  p -> willpower = 2;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(PECK);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(PLUCK);
  DESCRIPTION(LUNGE);
  DESCRIPTION(DRILL_PECK);
  DESCRIPTION(UPROAR);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 2; // Swords Dance
  return p;
}

struct pokemon *new_dodrio(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dodrio");
  strcpy(p -> desc, "Pokémon Pássaro Triplo");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Cerulean Cave
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
    BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(PECK);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(PLUCK);
  DESCRIPTION(LUNGE);
  DESCRIPTION(TRI_ATTACK);
  DESCRIPTION(DRILL_PECK);
  DESCRIPTION(UPROAR);
  DESCRIPTION(THRASH);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 2; // Swords Dance
  return p;
}

struct pokemon *new_seel(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Seel");
  strcpy(p -> desc, "Pokémon Leão Marinho");
  p -> shape = FISH;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 1; // 35% Seafo Island
  p -> willpower = 2;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(AQUA_JET);
  DESCRIPTION(ICY_WIND);
  DESCRIPTION(AURORA_BEAM);
  DESCRIPTION(BRINE);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(AQUA_TAIL);
  END_DESCRIPTION();
  p -> hide_att_move = dive; p -> aim_att_type = WATER;
  p -> friendship_move = growl;
  return p;
}

struct pokemon *new_dewgong(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dewgong");
  strcpy(p -> desc, "Pokémon Leão Marinho");
  p -> shape = FISH;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 4; // 5% Seafo Island
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = ICE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(AQUA_JET);
  DESCRIPTION(ICY_WIND);
  DESCRIPTION(AURORA_BEAM);
  DESCRIPTION(BRINE);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(AQUA_TAIL);
  END_DESCRIPTION();
  p -> hide_att_move = dive; p -> aim_att_type = WATER;
  p -> friendship_move = growl;
  return p;
}

struct pokemon *new_grimer(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Grimer");
  strcpy(p -> desc, "Pokémon Lodo");
  p -> shape = HEAD_WITH_ARMS;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 50% Mansão
  p -> willpower = 2;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(SMOG);
  DESCRIPTION(POUND);
  DESCRIPTION(SLUDGE);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(SLUDGE_WAVE);
  DESCRIPTION(GUNK_SHOT);
  DESCRIPTION(BELCH);
  END_DESCRIPTION();
  p -> poison_move = poison_gas;
  p -> physical_soak = 2; // Acid Armor
  return p;
}

struct pokemon *new_muk(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Muk");
  strcpy(p -> desc, "Pokémon Lodo");
  p -> shape = HEAD_WITH_ARMS;
  p -> vitality = 11; //105
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -4; p -> penalty[10] = -4; p -> penalty[11] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 3; // 15% Mansão
  p -> willpower = 7;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
    BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(SMOG);
  DESCRIPTION(POUND);
  DESCRIPTION(SLUDGE);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(SLUDGE_WAVE);
  DESCRIPTION(GUNK_SHOT);
  DESCRIPTION(BELCH);
  END_DESCRIPTION();
  p -> poison_move = poison_gas;
  p -> physical_soak = 2; // Acid Armor
  return p;
}


struct pokemon *new_shellder(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Shellder");
  strcpy(p -> desc, "Pokémon Bivalve");
  p -> shape = HEAD;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 0; p -> perception = 0; // 60% Rota 18
  p -> willpower = 2;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Ice Shard
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(AURORA_BEAM);
  DESCRIPTION(RAZOR_SHELL);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> grapple_move = whirlpool;
  p -> grapple_type = WATER;
  p ->  grapple_category = SPECIAL;
  p -> other_senses = true;
  p -> scare_move  = leer;
  p -> physical_soak = 2; // Iron Defense
  return p;
}

struct pokemon *new_cloyster(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Cloyster");
  strcpy(p -> desc, "Pokémon Bivalve");
  p -> shape = HEAD;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 5;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 5; // None
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = ICE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Ice Shard
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ICICLE_SPEAR);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(AURORA_BEAM);
  DESCRIPTION(RAZOR_SHELL);
  DESCRIPTION(ICICLE_CRASH);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> dif_terrain_move = spikes;
  p -> grapple_move = whirlpool;
  p -> grapple_type = WATER;
  p ->  grapple_category = SPECIAL;
  p -> other_senses = true;
  p -> scare_move  = leer;
  p -> physical_soak = 2; // Iron Defense
  return p;
}

struct pokemon *new_gastly(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Gastly");
  strcpy(p -> desc, "Pokémon Gás");
  p -> shape = HEAD;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 0;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 0;
  p -> inteligence = 3; p -> wits = 1; p -> perception = 0; // 90%: Pokemon Tower
  p -> willpower = 2;
  p -> type1 = GHOST; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(PAYBACK);
  DESCRIPTION(HEX);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(DARK_PULSE);
  DESCRIPTION(SHADOW_BALL);
  END_DESCRIPTION();
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_haunter(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Haunter");
  strcpy(p -> desc, "Pokémon Gás");
  p -> shape = HEAD_WITH_ARMS;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 1;
  p -> inteligence = 3; p -> wits = 1; p -> perception = 2; // 15%: Pokemon Tower
  p -> willpower = 6;
  p -> type1 = GHOST; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(PAYBACK);
  DESCRIPTION(SHADOW_PUNCH);
  DESCRIPTION(HEX);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(DARK_PULSE);
  DESCRIPTION(SHADOW_BALL);
  END_DESCRIPTION();
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_gengar(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Gengar");
  strcpy(p -> desc, "Pokémon Sombra");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 3; p -> appearance = 2;
  p -> inteligence = 4; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = GHOST; p -> type2 = POISON;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(PAYBACK);
  DESCRIPTION(SHADOW_PUNCH);
  DESCRIPTION(HEX);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(DARK_PULSE);
  DESCRIPTION(SHADOW_BALL);
  END_DESCRIPTION();
  p -> sleep_move = hypnosis;
  return p;
}


struct pokemon *new_onix(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Onix");
  strcpy(p -> desc, "Pokémon Cobra de Pedra");
  p -> shape = SERPENTINE;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 5;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 1; p -> perception = 1; // 30% Victory Road
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = GROUND;
  p -> size = LEGENDARY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(ROCK_THROW);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(ROCK_SLIDE);
  DESCRIPTION(IRON_TAIL);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> dif_terrain_move = stealth_rock;
  p -> grapple_move = bind;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> hide_att_move = dig; p -> aim_att_type = GROUND;
  p -> accuracy = 2; // Rock Polish
  p -> physical_soak = 1; // Harden
  return p;
}


struct pokemon *new_drowzee(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Drowzee");
  strcpy(p -> desc, "Pokémon Hipnose");
  p -> shape = HUMANOID;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 2; // 25%, rota 11
  p -> willpower = 2;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(FUTURE_SIGHT);
  END_DESCRIPTION();
  p -> poison_move = poison_gas;
  p -> special_damage = 2; // Nasty Plot
  p -> sleep_move = hypnosis;
  p -> mental_attack = true; // Psyshock
  return p;
}

struct pokemon *new_hypno(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Hypno");
  strcpy(p -> desc, "Pokémon Hipnose");
  p -> shape = HUMANOID;
  p -> vitality = 8; //85
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 2; // 15%, Cerulean Cave
  p -> willpower = 7;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(FUTURE_SIGHT);
  END_DESCRIPTION();
  p -> poison_move = poison_gas;
  p -> special_damage = 2; // Nasty Plot
  p -> sleep_move = hypnosis;
  p -> mental_attack = true; // Psyshock
  return p;
}


struct pokemon *new_krabby(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Krabby");
  strcpy(p -> desc, "Pokémon Caranguejo");
  p -> shape = INSECTOID;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 3; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 0; // 70% rota 25
  p -> willpower = 1;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(METAL_CLAW);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(STOMP);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(RAZOR_SHELL);
  DESCRIPTION(CRABHAMMER);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> scare_move  = leer;
  p -> physical_damage = 2; // Swords Dance
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_kingler(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kingler");
  strcpy(p -> desc, "Pokémon com Pinças");
  p -> shape = INSECTOID;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 4; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 30% rota 25
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(METAL_CLAW);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(STOMP);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(RAZOR_SHELL);
  DESCRIPTION(HAMMER_ARM);
  DESCRIPTION(CRABHAMMER);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> scare_move  = leer;
  p -> physical_damage = 2; // Swords Dance
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_voltorb(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Voltorb");
  strcpy(p -> desc, "Pokémon Bola");
  p -> shape = HEAD;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 0; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 45% rota 10
  p -> willpower = 2;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(CHARGE_BEAM);
  DESCRIPTION(SWIFT);
  DESCRIPTION(SPARK);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> special_soak = 1; // Charge
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  return p;
}

struct pokemon *new_electrode(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Electrode");
  strcpy(p -> desc, "Pokémon Bola");
  p -> shape = HEAD;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 4; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 3; // 15% Cerulean Cave
  p -> willpower = 7;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(CHARGE_BEAM);
  DESCRIPTION(SWIFT);
  DESCRIPTION(SPARK);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> special_soak = 1; // Charge
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  return p;
}


struct pokemon *new_exeggcute(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Exeggcute");
  strcpy(p -> desc, "Pokémon Ovo");
  p -> shape = MULTIPLE_BODIES;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 20% Safari Zone
  p -> willpower = 6;
  p -> type1 = GRASS; p -> type2 = PSYCHIC;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(BULLET_SEED);
  DESCRIPTION(ABSORB);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(EXTRASENSORY);
  DESCRIPTION(UPROAR);
  END_DESCRIPTION();
  p -> aim_att_move = solar_beam; p -> aim_att_type = GRASS;
  p -> aim_att_ranged = true;
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_exeggutor(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Exeggutor");
  strcpy(p -> desc, "Pokémon Coqueiro");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 9; //95
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = PSYCHIC;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(BULLET_SEED);
  DESCRIPTION(ABSORB);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(STOMP);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(EXTRASENSORY);
  DESCRIPTION(SEED_BOMB);
  DESCRIPTION(UPROAR);
  DESCRIPTION(WOOD_HAMMER);
  DESCRIPTION(LEAF_STORM);
  END_DESCRIPTION();
  p -> mental_attack = true;
  p -> aim_att_move = solar_beam; p -> aim_att_type = GRASS;
  p -> aim_att_ranged = true;
  p -> sleep_move = hypnosis;
  return p;
}

struct pokemon *new_cubone(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Cubone");
  strcpy(p -> desc, "Pokémon Solitário");
  p -> shape = BIPEDAL;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 3; // 10% Pokemon Tower
  p -> willpower = 2;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(BONE_RUSH);
  DESCRIPTION(FALSE_SWIPE);
  DESCRIPTION(BOOMERANG);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(STOMPING_TANTRUM);
  DESCRIPTION(THRASH);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  return p;
}

struct pokemon *new_marowak(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Marowak");
  strcpy(p -> desc, "Pokémon Portador do Osso");
  p -> shape = BIPEDAL;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 2; // 15% Cerulean Cave
  p -> willpower = 7;
  p -> type1 = GROUND; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(MUD_SLAP);
  DESCRIPTION(BONE_RUSH);
  DESCRIPTION(FALSE_SWIPE);
  DESCRIPTION(BOOMERANG);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(RETALIATE);
  DESCRIPTION(STOMPING_TANTRUM);
  DESCRIPTION(THRASH);
  DESCRIPTION(DOUBLE_EDGE);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  return p;
}

struct pokemon *new_hitmonlee(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Hitmonlee");
  strcpy(p -> desc, "Pokémon Chutador");
  p -> shape = HUMANOID;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FEINT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(REVENGE);
  DESCRIPTION(LOW_SWEEP);
  DESCRIPTION(BRICK_BREAK);
  DESCRIPTION(BLAZE_KICK);
  DESCRIPTION(MEGA_KICK);
  DESCRIPTION(CLOSE_COMBAT);
  DESCRIPTION(HIGH_JUMP_KICK);
  END_DESCRIPTION();
  p -> ambush_move = fake_out;
  p ->  ambush_type = NORMAL;
  p ->  ambush_attack = WITHERING_ATTACK;
  return p;
}


struct pokemon *new_hitmonchan(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Hitmonchan");
  strcpy(p -> desc, "Pokémon Socador");
  p -> shape = HUMANOID;
  p -> vitality = 5; //50
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = FIGHTING; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Bullet Punch
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(FEINT);
  DESCRIPTION(BULLET_PUNCH);
  DESCRIPTION(TACKLE);
  DESCRIPTION(VACUUM_WAVE);
  DESCRIPTION(MACH_PUNCH);
  DESCRIPTION(POWER_UP_PUNCH);
  DESCRIPTION(REVENGE);
  DESCRIPTION(THUNDER_PUNCH);
  DESCRIPTION(ICE_PUNCH);
  DESCRIPTION(FIRE_PUNCH);
  DESCRIPTION(DRAIN_PUNCH);
  DESCRIPTION(MEGA_PUNCH);
  DESCRIPTION(CLOSE_COMBAT);
  DESCRIPTION(FOCUS_PUNCH);
  END_DESCRIPTION();
  p -> full_defense_move = detect;
  p -> ambush_move = fake_out;
  p ->  ambush_type = NORMAL;
  p ->  ambush_attack = WITHERING_ATTACK;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_lickitung(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Lickitung");
  strcpy(p -> desc, "Pokémon Lambedor");
  p -> shape = BIPEDAL;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 4; // 5% Cerulean Cave
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(STOMP);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(POWER_WHIP);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> other_senses = true;
  p -> physical_soak = 1; // Defense Curl
  return p;
}


struct pokemon *new_koffing(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Koffing");
  strcpy(p -> desc, "Pokémon Gás Venenoso");
  p -> shape = HEAD;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 50% Pokemon Mansion
  p -> willpower = 2;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SMOG);
  DESCRIPTION(TACKLE);
  DESCRIPTION(CLEAR_SMOG);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(SLUDGE);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(BELCH);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> poison_move = poison_gas; p -> poison_remaining = 1;
  return p;
}

struct pokemon *new_weezing(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Weezing");
  strcpy(p -> desc, "Pokémon Gás Venenoso");
  p -> shape = MULTIPLE_BODIES;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 2; // 15% Pokemon Mansion
  p -> willpower = 7;
  p -> type1 = POISON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SMOG);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(TACKLE);
  DESCRIPTION(CLEAR_SMOG);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(SLUDGE);
  DESCRIPTION(SLUDGE_BOMB);
  DESCRIPTION(HEAT_WAVE);
  DESCRIPTION(BELCH);
  DESCRIPTION(EXPLOSION);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> poison_move = poison_gas; p -> poison_remaining = 2;
  return p;
}

struct pokemon *new_rhyhorn(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Rhyhorn");
  strcpy(p -> desc, "Pokémon Espinhoso");
  p -> shape = QUADRUPED;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 2; // 15% Safari Zone
  p -> willpower = 5;
  p -> type1 = GROUND; p -> type2 = ROCK;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(TACKLE);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(STOMP);
  DESCRIPTION(DRILL_RUN);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(EARTHQUAKE);
  DESCRIPTION(MEGAHORN);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> scare_move = scary_face;
  p -> friendship_move = tail_whip;
  return p;
}

struct pokemon *new_rhydon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Rhydon");
  strcpy(p -> desc, "Pokémon Furadeiro");
  p -> shape = BIPEDAL;
  p -> vitality = 11; //105
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -4; p -> penalty[10] = -4; p -> penalty[11] = -4;
  p -> essence = 2;
  p -> strength = 4; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Cerulean Cave
  p -> willpower = 7;
  p -> type1 = GROUND; p -> type2 = ROCK;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(TACKLE);
  DESCRIPTION(SMACK_DOWN);
  DESCRIPTION(BULLDOZE);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(STOMP);
  DESCRIPTION(DRILL_RUN);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(HAMMER_ARM);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(EARTHQUAKE);
  DESCRIPTION(MEGAHORN);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> scare_move = scary_face;
  p -> friendship_move = tail_whip;
  return p;
}

struct pokemon *new_chansey(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Chansey");
  strcpy(p -> desc, "Pokémon Ovo");
  p -> shape = BIPEDAL;
  p -> vitality = 25; //250
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = 0; p -> penalty[4] = 0; p -> penalty[5] = -1;
  p -> penalty[6] = -1; p -> penalty[7] = -1; p -> penalty[8] = -1;
  p -> penalty[9] = -1; p -> penalty[10] = -1; p -> penalty[11] = -1;
  p -> penalty[12] = -1; p -> penalty[13] = -2; p -> penalty[14] = -2;
  p -> penalty[15] = -2; p -> penalty[16] = -2; p -> penalty[17] = -2;
  p -> penalty[18] = -2; p -> penalty[19] = -2; p -> penalty[20] = -2;
  p -> penalty[21] = -2; p -> penalty[22] = -4; p -> penalty[23] = -4;
  p -> penalty[24] = -4; p -> penalty[25] = -4;
  p -> essence = 2;
  p -> strength = 0; p -> dexterity = 1; p -> stamina = 0;
  p -> charisma = 5; p -> manipulation = 1; p -> appearance = 5;
  p -> inteligence = 1; p -> wits = 3; p -> perception = 3; // 10% Cerulean Cave
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DISARMING_VOICE);
  DESCRIPTION(POUND);
  DESCRIPTION(ECHOED_VOICE);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = sweet_kiss;
  p -> physical_soak = 1; // Defense Curl
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_tangela(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Tangela");
  strcpy(p -> desc, "Pokémon Vinhas");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 5; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 3; p -> wits = 1; p -> perception = 3; // 10% Rota 21
  p -> willpower = 8;
  p -> type1 = GRASS; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(MEGA_DRAIN);
  DESCRIPTION(VINE_WHIP);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(KNOCK_OFF);
  DESCRIPTION(GIGA_DRAIN);
  DESCRIPTION(POWER_WHIP);
  END_DESCRIPTION();
  p -> grapple_move = bind;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> friendship_move = tickle;
  p -> poison_move = poison_powder; p -> poison_remaining = 2;
  p -> paralysis_move = stun_spore;
  p -> physical_damage = 1; p -> special_damage = 1; // Growth
  p -> sleep_move = sleep_powder;
  return p;
}

struct pokemon *new_kangaskhan(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kangaskhan");
  strcpy(p -> desc, "Pokémon Parental");
  p -> shape = BIPEDAL;
  p -> vitality = 11; //105
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -4; p -> penalty[10] = -4; p -> penalty[11] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 5; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 2; // 15% Safari Zone
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(POUND);
  DESCRIPTION(BITE);
  DESCRIPTION(STOMP);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(HEADBUTT);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> ambush_move = fake_out;
  p ->  ambush_type = NORMAL;
  p ->  ambush_attack = WITHERING_ATTACK;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  return p;
}

struct pokemon *new_staryu(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Staryu");
  strcpy(p -> desc, "Pokémon com Forma de Estrela");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 5; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 0; // 60% Cinnabar Island
  p -> willpower = 1;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(SWIFT);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(BRINE);
  DESCRIPTION(POWER_GEM);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(SURF);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  p -> physical_soak = 1; p -> special_soak = 1; // Cosmic Power
  return p;
}

struct pokemon *new_starmie(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Starmie");
  strcpy(p -> desc, "Pokémon Misterioso");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 6; //60
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 5; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = PSYCHIC;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(RAPID_SPIN);
  DESCRIPTION(SWIFT);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(BRINE);
  DESCRIPTION(POWER_GEM);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(SURF);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> genderless = true;
  p -> physical_soak = 1; p -> special_soak = 1; // Cosmic Power
  return p;
}

struct pokemon *new_eevee(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Eevee");
  strcpy(p -> desc, "Pokémon Evolução");
  p -> shape = QUADRUPED;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(COVET);  
  DESCRIPTION(SWIFT);
  DESCRIPTION(BITE);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = baby_doll_eyes;
  return p;
}

struct pokemon *new_vaporeon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Vaporeon");
  strcpy(p -> desc, "Pokémon do Jato de Bolhas");
  p -> shape = QUADRUPED;
  p -> vitality = 13; //130
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -1; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -2; p -> penalty[10] = -2; p -> penalty[11] = -2;
  p -> penalty[12] = -4; p -> penalty[13] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(COVET);  
  DESCRIPTION(SWIFT);
  DESCRIPTION(BITE);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(AURORA_BEAM);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(MUDDY_WATER);
  DESCRIPTION(HYDRO_PUMP);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = baby_doll_eyes;
  p -> physical_soak = 2; // Acid Armor
  return p;
}

struct pokemon *new_jolteon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Jolteon");
  strcpy(p -> desc, "Pokémon do Raio");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 4; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(PIN_MISSILE);
  DESCRIPTION(DOUBLE_KICK);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(COVET);  
  DESCRIPTION(SWIFT);
  DESCRIPTION(BITE);
  DESCRIPTION(THUNDER_FANG);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(THUNDER);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = baby_doll_eyes;
  p -> paralysis_move = thunder_wave;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_flareon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Flareon");
  strcpy(p -> desc, "Pokémon da Chama");
  p -> shape = QUADRUPED;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 4; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SMOG);
  DESCRIPTION(TACKLE);
  DESCRIPTION(EMBER);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(COVET);  
  DESCRIPTION(SWIFT);
  DESCRIPTION(BITE);
  DESCRIPTION(FIRE_FANG);
  DESCRIPTION(LAVA_PLUME);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(FLARE_BLITZ);
  DESCRIPTION(LAST_RESORT);
  END_DESCRIPTION();
  p -> smoke_move = sand_attack;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  p -> friendship_move = growl;
  p -> friendship2 = tail_whip;
  p -> flirt_move = charm;
  p -> flirt2 = baby_doll_eyes;
  return p;
}


struct pokemon *new_horsea(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Horsea");
  strcpy(p -> desc, "Pokémon Dragão");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 0; p -> perception = 0; // 70% Rota 12
  p -> willpower = 1;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(DRAGON_PULSE);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Dragon Dance
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_seadra(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Seadra");
  strcpy(p -> desc, "Pokémon Dragão");
  p -> shape = HEAD_WITH_BASE;
  p -> vitality = 6; //55
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 1; // 30% Rota 12
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(DRAGON_BREATH);
  DESCRIPTION(BUBBLE_BEAM);
  DESCRIPTION(DRAGON_PULSE);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Dragon Dance
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_goldeen(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Goldeen");
  strcpy(p -> desc, "Pokémon Peixe Dourado");
  p -> shape = FISH;
  p -> vitality = 5; //45
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 0; // 100% Rota 6
  p -> willpower = 1;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(PECK);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(WATERFALL);
  DESCRIPTION(MEGAHORN);
  END_DESCRIPTION();
  p -> other_senses = true;
  p -> friendship_move = tail_whip;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_seaking(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Seaking");
  strcpy(p -> desc, "Pokémon Peixe Dourado");
  p -> shape = FISH;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 0; // 60% Cerulean Cave
  p -> willpower = 7;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(PECK);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(WATERFALL);
  DESCRIPTION(MEGAHORN);
  END_DESCRIPTION();
  p -> other_senses = true;
  p -> friendship_move = tail_whip;
  p -> accuracy = 2; // Agility
  return p;
}


struct pokemon *new_mrmime(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Mr. Mime");
  strcpy(p -> desc, "Pokémon de Barreiras");
  p -> shape = HUMANOID;
  p -> vitality = 4; //40
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = PSYCHIC; p -> type2 = FAIRY;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(SUCKER_PUNCH);
  DESCRIPTION(DAZZLING_GLEAM);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> take_cover_move = light_screen;
  return p;
}

struct pokemon *new_scyther(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Scyther");
  strcpy(p -> desc, "Pokémon Louva-a-Deus");
  p -> shape = FLYING_BUG;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 1;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 4; // 4% Safari Zone
  p -> willpower = 8;
  p -> type1 = BUG; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(FURY_ATTACK);
  DESCRIPTION(FALSE_SWIPE);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(SLASH);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(X_SCISSOR);
  END_DESCRIPTION();
  p -> scare_move = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 2; // Swords Dance
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_jynx(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Jynx");
  strcpy(p -> desc, "Pokémon com Forma Humana");
  p -> shape = HUMANOID;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ICE; p -> type2 = PSYCHIC;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(POUND);
  DESCRIPTION(POWDER_SNOW);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(COVET);
  DESCRIPTION(ICE_PUNCH);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(BLIZZARD);
  END_DESCRIPTION();
  p -> friendship_move = fake_tears;
  p -> flirt_move = sweet_kiss;
  p -> seduce_move = lovely_kiss;
  p -> sleep_move = sing;
  return p;
}


struct pokemon *new_electabuzz(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Electabuzz");
  strcpy(p -> desc, "Pokémon Elétrico");
  p -> shape = BIPEDAL;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 3; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 4; // 5% Power Plant
  p -> willpower = 8;
  p -> type1 = ELECTRIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Quick Attack
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(QUICK_ATTACK);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(SWIFT);
  DESCRIPTION(SHOCK_WAVE);
  DESCRIPTION(THUNDER_PUNCH);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(THUNDERBOLT);
  DESCRIPTION(THUNDER);
  DESCRIPTION(GIGA_IMPACT);
  END_DESCRIPTION();
  p -> take_cover_move = light_screen;
  p -> scare_move  = leer;
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_magmar(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Magmar");
  strcpy(p -> desc, "Pokémon Cuspidor de Fogo");
    p -> shape = BIPEDAL;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 3; // 10% Mansion
  p -> willpower = 8;
  p -> type1 = FIRE; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(SMOG);
  DESCRIPTION(EMBER);
  DESCRIPTION(CLEAR_SMOG);
  DESCRIPTION(FLAME_WHEEL);
  DESCRIPTION(FIRE_PUNCH);
  DESCRIPTION(LAVA_PLUME);
  DESCRIPTION(FLAMETHROWER);
  DESCRIPTION(FIRE_BLAST);
  DESCRIPTION(HYPER_BEAM);
  END_DESCRIPTION();
  p -> smoke_move = smokescreen;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  return p;
}

struct pokemon *new_pinsir(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Pinsir");
  strcpy(p -> desc, "Pokémon Besouro Lucano");
  p -> shape = HUMANOID;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 4; // 4% Safari Zone
  p -> willpower = 8;
  p -> type1 = BUG; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(DOUBLE_HIT);
  DESCRIPTION(VISE_GRIP);
  DESCRIPTION(STORM_THROW);
  DESCRIPTION(BUG_BITE);
  DESCRIPTION(VITAL_THROW);
  DESCRIPTION(STRENGTH);
  DESCRIPTION(SUBMISSION);
  DESCRIPTION(X_SCISSOR);
  DESCRIPTION(SUPERPOWER);
  END_DESCRIPTION();
  p -> grapple_move = bind;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = seismic_toss;
  p -> slam_type = FIGHTING;
  p -> physical_damage = 2; // Swords Dance
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_tauros(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Tauros");
  strcpy(p -> desc, "Pokémon Touro Selvagem");
  p -> shape = QUADRUPED;
  p -> vitality = 8; //75
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 3; // 10% Safari Zone
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(PAYBACK);
  DESCRIPTION(ASSURANCE);
  DESCRIPTION(HORN_ATTACK);
  DESCRIPTION(ZEN_HEADBUTT);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(THRASH);
  DESCRIPTION(DOUBLE_EDGE);
  DESCRIPTION(GIGA_IMPACT);
  END_DESCRIPTION();
  p -> scare_move = scary_face;
  p -> friendship_move = tail_whip;
  p -> physical_damage = 1; p -> special_damage = 1; // Bulk Up
  p -> anger_move = swagger;
  return p;
}


struct pokemon *new_magikarp(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Magikarp");
  strcpy(p -> desc, "Pokémon Peixe");
  p -> shape = FISH;
  p -> vitality = 2; //20
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> essence = 1;
  p -> strength = 0; p -> dexterity = 2; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 0; p -> wits = 0; p -> perception = 0; // 100% Rota 6
  p -> willpower = 0;
  p -> type1 = WATER; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  END_DESCRIPTION();
  return p;
}

struct pokemon *new_gyarados(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Gyarados");
  strcpy(p -> desc, "Pokémon Atroz");
  p -> shape = SERPENTINE;
  p -> vitality = 9; //95
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 3; p -> perception = 3; // 10% Fuchsia City
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = FLYING;
  p -> size = LEGENDARY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(BITE);
  DESCRIPTION(ICE_FANG);
  DESCRIPTION(BRINE);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(WATERFALL);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(HYDRO_PUMP);
  DESCRIPTION(HURRICANE);
  DESCRIPTION(THRASH);
  DESCRIPTION(HYPER_BEAM);
  END_DESCRIPTION();
  p -> grapple_move = whirlpool;
  p -> grapple_type = WATER;
  p ->  grapple_category = SPECIAL;
  p -> scare_move = scary_face;
  p -> physical_damage = 1; p -> accuracy = 1; // Dragon Dance
  return p;
}

struct pokemon *new_lapras(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Lapras");
  strcpy(p -> desc, "Pokémon de Transporte");
  p -> shape = FISH;
  p -> vitality = 13; //130
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -1; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -2; p -> penalty[10] = -2; p -> penalty[11] = -2;
  p -> penalty[12] = -4; p -> penalty[13] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = WATER; p -> type2 = ICE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Ice Shard
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(WATER_PULSE);
  DESCRIPTION(BRINE);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> friendship_move = growl;
  p -> sleep_move = sing;
  return p;
}

struct pokemon *new_ditto(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Ditto");
  strcpy(p -> desc, "Pokémon da Transformação");
  p -> shape = HEAD;
  p -> vitality = 5; //48
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 1; // 30% Rota 23
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NORMAL;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  END_DESCRIPTION();
  p -> genderless = true;
  p -> can_transform = true;
  return p;
}


struct pokemon *new_porygon(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Porygon");
  strcpy(p -> desc, "Pokémon Virtual");
  p -> shape = HEAD_WITH_LEGS;
  p -> vitality = 7; //65
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4; p -> penalty[7] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TACKLE);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(TRI_ATTACK);
  DESCRIPTION(ZAP_CANNON);
  END_DESCRIPTION();
  p -> aim_move = lock_on;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_omanyte(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Omanyte");
  strcpy(p -> desc, "Pokémon Espiral");
  p -> shape = OCTOPUS;
  p -> vitality = 4; //35
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -2; p -> penalty[4] = -4;
  p -> essence = 1;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 2; p -> wits = 1; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = WATER;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(BRINE);
  DESCRIPTION(SURF);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> smoke_move = sand_attack;
  p -> grapple_move = bind;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> scare_move  = leer;
  p -> physical_soak = 1; // Withdraw
  return p;
}

struct pokemon *new_omastar(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Omastar");
  strcpy(p -> desc, "Pokémon Espiral");
  p -> shape = OCTOPUS;
  p -> vitality = 7; //70
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -4;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 1; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = WATER;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ROCK_BLAST);
  DESCRIPTION(ROLLOUT);
  DESCRIPTION(WATER_GUN);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(BRINE);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(SURF);
  DESCRIPTION(HYDRO_PUMP);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> smoke_move = sand_attack;
  p -> grapple_move = bind;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> scare_move  = leer;
  p -> physical_soak = 1; // Withdraw
  return p;
}

struct pokemon *new_kabuto(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kabuto");
  strcpy(p -> desc, "Pokémon Marisco");
  p -> shape = INSECTOID;
  p -> vitality = 3; //30
  p -> penalty[0] = 0; p -> penalty[1] = -1; p -> penalty[2] = -2;
  p -> penalty[3] = -4;
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = WATER;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(AQUA_JET);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(BRINE);
  DESCRIPTION(LEECH_LIFE);
  DESCRIPTION(LIQUIDATION);
  DESCRIPTION(STONE_EDGE);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> smoke_move = sand_attack;
  p -> scare_move  = leer;
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_kabutops(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Kabutops");
  strcpy(p -> desc, "Pokémon Marisco");  p -> vitality = 6; //60
  p -> shape = BIPEDAL;
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 2; p -> manipulation = 2; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = WATER;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Aqua Jet
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(ABSORB);
  DESCRIPTION(FEINT);
  DESCRIPTION(SCRATCH);
  DESCRIPTION(AQUA_JET);
  DESCRIPTION(MUD_SHOT);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(BRINE);
  DESCRIPTION(SLASH);
  DESCRIPTION(NIGHT_SLASH);
  DESCRIPTION(LEECH_LIFE);
  DESCRIPTION(LIQUIDATION);
  DESCRIPTION(STONE_EDGE);
  END_DESCRIPTION();
  p -> full_defense_move = protect;
  p -> smoke_move = sand_attack;
  p -> scare_move  = leer;
  p -> physical_soak = 1; // Harden
  return p;
}

struct pokemon *new_aerodactyl(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Aerodactyl");
  strcpy(p -> desc, "Pokémon Fóssil");
  p -> shape = BIRD;
  p -> vitality = 8; //80
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 4; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = ROCK; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(BITE);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(ROCK_SLIDE);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(IRON_HEAD);
  DESCRIPTION(TAKE_DOWN);
  DESCRIPTION(STONE_EDGE);
  DESCRIPTION(HYPER_BEAM);
  DESCRIPTION(GIGA_IMPACT);
  END_DESCRIPTION();
  p -> other_senses = true;
  p -> threaten_move = roar;
  p -> scare_move = scary_face;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_snorlax(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Snorlax");
  strcpy(p -> desc, "Pokémon Dorminhoco");
  p -> shape = HUMANOID;
  p -> vitality = 16; //160
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -1; p -> penalty[7] = -1; p -> penalty[8] = -1;
  p -> penalty[9] = -2; p -> penalty[10] = -2; p -> penalty[11] = -2;
  p -> penalty[12] = -2; p -> penalty[13] = -2; p -> penalty[14] = -2;
  p -> penalty[15] = -4; p -> penalty[16] = -4;
  p -> essence = 2;
  p -> strength = 3; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 2; p -> manipulation = 4; p -> appearance = 2;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = NORMAL; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(LICK);
  DESCRIPTION(TACKLE);
  DESCRIPTION(SNORE);
  DESCRIPTION(COVET);
  DESCRIPTION(CRUNCH);
  DESCRIPTION(BODY_SLAM);
  DESCRIPTION(HIGH_HORSEPOWER);
  DESCRIPTION(HAMMER_ARM);
  DESCRIPTION(BELCH);
  DESCRIPTION(LAST_RESORT);
  DESCRIPTION(GIGA_IMPACT);
  END_DESCRIPTION();
  p -> physical_soak = 1; // Defense Curl
  p -> special_soak = 2; // Amnesia
  p -> sleep_move = yawn;
  return p;
}


struct pokemon *new_articuno(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Articuno");
  strcpy(p -> desc, "Pokémon do Congelamento");
  p -> shape = BIRD;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = ICE; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Ice Shard
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(GUST);
  DESCRIPTION(POWDER_SNOW);
  DESCRIPTION(ICE_SHARD);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(FREEZE_DRY);
  DESCRIPTION(ICE_BEAM);
  DESCRIPTION(HURRICANE);
  DESCRIPTION(BLIZZARD);
  END_DESCRIPTION();
  p -> genderless = true;
  p -> accuracy = 2; // Agility
  return p;
}


struct pokemon *new_zapdos(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Zapdos");
  strcpy(p -> desc, "Pokémon Elétrico");
  p -> shape = BIRD;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 2;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = ELECTRIC; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(PECK);
  DESCRIPTION(THUNDER_SHOCK);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(PLUCK);
  DESCRIPTION(DRILL_PECK);
  DESCRIPTION(DISCHARGE);
  DESCRIPTION(THUNDER);
  DESCRIPTION(ZAP_CANNON);
  END_DESCRIPTION();
  p -> full_defense_move = detect;
  p -> take_cover_move = light_screen;
  p -> accuracy = 2; // Agility
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_moltres(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Moltres");
  strcpy(p -> desc, "Pokémon da Chama");
  p -> shape = BIRD;
  p -> vitality = 9; //90
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = FIRE; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(GUST);
  DESCRIPTION(EMBER);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(INCINERATE);
  DESCRIPTION(AIR_SLASH);
  DESCRIPTION(HEAT_WAVE);
  DESCRIPTION(HURRICANE);
  DESCRIPTION(BURN_UP);
  END_DESCRIPTION();
  p -> aim_att_move = sky_attack; p -> aim_att_type = FLYING;
  p -> aim_att_ranged = true;
  p -> grapple_move = fire_spin;
  p -> grapple_type = FIRE;
  p ->  grapple_category = SPECIAL;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  return p;
}

struct pokemon *new_dratini(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dratini");
  strcpy(p -> desc, "Pokémon Dragão");
  p -> shape = SERPENTINE;
  p -> vitality = 4; //41
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -2; p -> penalty[4] = -4; 
  p -> essence = 1;
  p -> strength = 2; p -> dexterity = 1; p -> stamina = 1;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 0;
  p -> inteligence = 1; p -> wits = 1; p -> perception = 2; // 25% Safari Zone
  p -> willpower = 8;
  p -> type1 = DRAGON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(DRAGON_RUSH);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(HYPER_BEAM);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Dragon Dance
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_dragonair(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dragonair");
  strcpy(p -> desc, "Pokémon Dragão");
  p -> shape = SERPENTINE;
  p -> vitality = 6; //61
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> essence = 2;
  p -> strength = 2; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 1;
  p -> inteligence = 2; p -> wits = 2; p -> perception = 3; // 10% Safari Zone
  p -> willpower = 8;
  p -> type1 = DRAGON; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(DRAGON_RUSH);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(HYPER_BEAM);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Dragon Dance
  p -> paralysis_move = thunder_wave;
  return p;
}

struct pokemon *new_dragonite(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dragonite");
  strcpy(p -> desc, "Pokémon Dragão");
  p -> shape = BIPEDAL;
  p -> vitality = 9; //91
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -2;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -4;
  p -> penalty[9] = -4;
  p -> essence = 3;
  p -> strength = 4; p -> dexterity = 2; p -> stamina = 2;
  p -> charisma = 1; p -> manipulation = 4; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = DRAGON; p -> type2 = FLYING;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence + 1); // Extreme Speed
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(TWISTER);
  DESCRIPTION(WING_ATTACK);
  DESCRIPTION(FIRE_PUNCH);
  DESCRIPTION(THUNDER_PUNCH);
  DESCRIPTION(EXTREME_SPEED);
  DESCRIPTION(AQUA_TAIL);
  DESCRIPTION(DRAGON_RUSH);
  DESCRIPTION(HURRICANE);
  DESCRIPTION(OUTRAGE);
  DESCRIPTION(HYPER_BEAM);
  END_DESCRIPTION();
  p -> grapple_move = wrap;
  p -> grapple_type = NORMAL;
  p ->  grapple_category = PHYSICAL;
  p -> slam_move = slam;
  p -> slam_type = NORMAL;
  p -> scare_move  = leer;
  p -> accuracy = 2; // Agility
  p -> physical_damage = 1; // Dragon Dance
  p -> paralysis_move = thunder_wave;
  return p;
}


struct pokemon *new_mewtwo(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Mewtwo");
  strcpy(p -> desc, "Pokémon Genético");
  p -> shape = BIPEDAL;
  p -> vitality = 11; //106
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -4; p -> penalty[10] = -4; p -> penalty[11] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 4; p -> stamina = 2;
  p -> charisma = 0; p -> manipulation = 4; p -> appearance = 3;
  p -> inteligence = 4; p -> wits = 2; p -> perception = 5; // None
  p -> willpower = 9;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = NORMAL;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(SWIFT);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(PSYCHO_CUT);
  DESCRIPTION(AURA_SPHERE);
  DESCRIPTION(PSYCHIC_ATT);
  DESCRIPTION(FUTURE_SIGHT);
  END_DESCRIPTION();
  p -> can_teleport = true;
  p -> mental_attack = true; // Psystrike
  p -> genderless = true;
  p -> special_soak = 2; // Amnesia
  p -> aim_move = laser_focus;
  return p;
}

struct pokemon *new_mew(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Mew");
  strcpy(p -> desc, "Pokémon de Novas Espécies");
  p -> shape = BIPEDAL;
  p -> vitality = 10; //100
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = 0;
  p -> penalty[3] = -1; p -> penalty[4] = -1; p -> penalty[5] = -1;
  p -> penalty[6] = -2; p -> penalty[7] = -2; p -> penalty[8] = -2;
  p -> penalty[9] = -4; p -> penalty[10] = -4;
  p -> essence = 3;
  p -> strength = 3; p -> dexterity = 3; p -> stamina = 3;
  p -> charisma = 3; p -> manipulation = 3; p -> appearance = 3;
  p -> inteligence = 3; p -> wits = 3; p -> perception = 5; // None
  p -> willpower = 8;
  p -> type1 = PSYCHIC; p -> type2 = NONE;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION(POUND);
  DESCRIPTION(ANCIENT_POWER);
  DESCRIPTION(AURA_SPHERE);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  p -> genderless = true;
  p -> can_transform = true;
  p -> special_damage = 2; // Nasty Plot
  p -> special_soak = 2; // Amnesia
  return p;
}


struct pokemon *new_blipbug(int team){
  struct pokemon *blipbug = new_base_pokemon(team);
  strcpy(blipbug -> name, "Blipbug");
  strcpy(blipbug -> desc, "Pokémon Larva");
  blipbug -> shape = INSECTOID;
  blipbug -> vitality = 2;
  blipbug -> penalty[0] = 0;
  blipbug -> penalty[1] = -1; blipbug -> penalty[2] = -2;
  blipbug -> damage = 0;
  blipbug -> essence = 1;
  blipbug -> strength = 0; blipbug -> dexterity = 1; blipbug -> stamina = 0;
  blipbug -> charisma = 1; blipbug -> manipulation = 2; blipbug -> appearance = 0;
  blipbug -> inteligence = 0; blipbug -> wits = 1; blipbug -> perception = 1;
  blipbug -> willpower = 0;
  blipbug -> type1 = BUG; blipbug -> type2 = NONE;
  blipbug -> team = team;
  blipbug -> size = TINY;
  blipbug -> initiative = roll(blipbug -> wits + blipbug -> essence);
  if(blipbug -> initiative < 0)
    blipbug -> initiative = 3;
  else
    blipbug -> initiative += 3;
  BEGIN_DESCRIPTION(blipbug);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION("Fúria de Inseto", BUG, true);
  END_DESCRIPTION();
  return blipbug;
}

struct pokemon *new_dottler(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Dottler");
  strcpy(p -> desc, "Pokémon Redoma");
  p -> shape = INSECTOID;
  p -> vitality = 5;
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -4;
  p -> damage = 0;
  p -> essence = 2;
  p -> strength = 1; p -> dexterity = 0; p -> stamina = 2;
  p -> charisma = 1; p -> manipulation = 2; p -> appearance = 1;
  p -> inteligence = 1; p -> wits = 2; p -> perception = 1;
  p -> willpower = 5;
  p -> type1 = BUG; p -> type2 = PSYCHIC;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION("Fúria de Inseto", BUG, true);
  DESCRIPTION(CONFUSION);
  END_DESCRIPTION();
  return p;
}

struct pokemon *new_orbeetle(int team){
  struct pokemon *p = new_base_pokemon(team);
  strcpy(p -> name, "Orbeetle");
  strcpy(p -> desc, "Pokémon dos Sete Pontos");
  p-> shape = BIRD;
  p -> vitality = 6;
  p -> penalty[0] = 0; p -> penalty[1] = 0; p -> penalty[2] = -1;
  p -> penalty[3] = -1; p -> penalty[4] = -2; p -> penalty[5] = -2;
  p -> penalty[6] = -4;
  p -> damage = 0;
  p -> essence = 3;
  p -> strength = 1; p -> dexterity = 2; p -> stamina = 3;
  p -> charisma = 1; p -> manipulation = 2; p -> appearance = 3;
  p -> inteligence = 2; p -> wits = 3; p -> perception = 1;
  p -> willpower = 8;
  p -> type1 = BUG; p -> type2 = PSYCHIC;
  p -> size = TINY;
  p -> team = team;
  p -> initiative = roll(p -> wits + p -> essence);
  if(p -> initiative < 0)
    p -> initiative = 3;
  else
    p -> initiative += 3;
  BEGIN_DESCRIPTION(p);
  DESCRIPTION(STRUGGLE);
  DESCRIPTION("Fúria de Inseto", BUG, true);
  DESCRIPTION(CONFUSION);
  DESCRIPTION(PSYBEAM);
  DESCRIPTION(BUG_BUZZ);
  DESCRIPTION(PSYCHIC_ATT);
  END_DESCRIPTION();
  return p;
}

int simulate_decisive_attack(struct pokemon *p1, struct pokemon *p2,
			     int extra_attack, int extra_def, int force_type,
			     int range){
  int damage, hardness = 0, i, attack_bonus = 0, defense_bonus = 0;
  int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1) + extra_attack);
  int defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1) + extra_def;
  i = (p1 -> initiative - 1) / 2;
  if(i > 25) i = 25;
  while(p1 -> description[i] == NULL)
    i --;
  if(range == DEFAULT){
    range = p1 -> ranged[i];
    if(p1 -> control_turns > 0)
      range = 0;
  }
  // Cobertura atrapalha ataque de curta distância:
  if(p1 -> covered > 0 && range == CLOSE)
    defense += ((p1 -> covered > 2)?(2):(p1 -> covered));
  if(defense < 0) defense = 0;
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    attack_bonus ++;
  if(p2 -> willpower > 0 && !(p2 -> grappled))
    defense_bonus ++;
#endif
  attack += attack_bonus;
  defense += defense_bonus;
  if(p2 -> grappled)
    defense = 0;
  p1 -> extra = attack - defense;
  if(force_type != NONE)
    hardness = compute_weakness(force_type, p2);
  else if(p1 -> control_turns > 0)
    hardness = compute_weakness(p1 -> grapple_type, p2);
  else
    hardness = compute_weakness(p1 -> type[i], p2);
  if(!force_type && !strcmp(p1 -> description[i], "Congelamento Rápido") &&
     (p2 -> type1 == WATER || p2 -> type2 == WATER))
    hardness -= 2;
  if(attack < defense || hardness > 2)
    return -1;
  damage = roll_no_double(p1 -> initiative) - hardness;
  if(damage < 0)
    damage = 0;
  if(p2 -> size == LEGENDARY && p1 -> size < LEGENDARY &&
     damage > p1 -> strength + 3)
    damage = p1 -> strength + 3;
  return damage;
}

void slam_att(struct pokemon *p1, struct pokemon *p2){
  int control = p1 -> control_turns, damage;
  if(p1 -> control_turns == 0 && !(p2 -> grappled)){
    printf("%s tenta se aproximar de %s.\n", p1 -> name, p2 -> name);
    damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> slam_type, 0);
    if(damage < 0){
      printf("Mas %s se desviou e %s perdeu 5 de Iniciativa.\n", p2 -> name,
	     p1 -> name);
      add_initiative(p1, p2, -5, true);
      return;
    }
    add_initiative(p1, p2, -3, true);
    if(damage < 2){
      printf("Mas não conseguiu segurar %s.\n", p2 -> name);
      return;
    }
    printf("Conseguiu!\n");
    control = roll(GET_STRENGTH_DICE(p1)) - roll(GET_STRENGTH_DICE(p2));
    if(control < 0)
      control = 1;
    p1 -> control_turns = control;
    p2 -> grappled = true;
  }
  printf("%s usou '%s' contra %s.\n", p1 -> name, p1 -> slam_move, p2 -> name);
  if(control - 1 > p1 -> strength)
    control = p1 -> strength + 1;
  p1 -> initiative += control - 1;
  damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> slam_type, 0);
  p2 -> grappled = false;
  p1 -> control_turns = 0;
  p1 -> initiative = 3;
  if(damage < 0)
    printf("%s conseguiu evitar o dano!\n", p2 -> name);
  else{
    int resistance = compute_weakness(p1 -> slam_type, p2);
    printf("Sofreu %d de dano.\n", damage);
    if(resistance < 0)
      printf("Foi super-efetivo!\n");
    else if(resistance > 0)
      printf("Não foi muito efetivo.\n");
    p2 -> damage += damage;
    if(p2 -> shape != HEAD && p2 -> shape != FISH && p2 -> shape != FLYING_BUG &&
       p2 -> shape != MULTIPLE_BODIES && p2 -> shape != OCTOPUS &&
       p2 -> shape != HEAD_WITH_BASE && p2 -> shape != BIRD &&
       p2 -> shape != HEAD_WITH_ARMS){
      if(p1 -> size > p2 -> size || (p2 -> shape != INSECTOID &&
				     p2 -> shape != QUADRUPED)){
	printf("%s está caído no chão.\n", p2 -> name);
	p2 -> prone = true;
      }
    }
    if(p2 -> damage > p2 -> vitality)
      printf("%s MORREU!!!! \n", p2 -> name);
  }
}

void hide_att(struct pokemon *p1, struct pokemon *p2){
  if(p1 -> hiding == false){
    int hide, seek;
    printf("%s tenta se esconder com %s.\n", p1 -> name, p1 -> hide_att_move);
    hide = roll(GET_ATTACK_DICE(p1) + GET_BONUS_STEALTH(p1) - 3);
    seek = roll(GET_PERCEPTION_DICE(p2) + GET_BONUS_PERCEPTION(p2));
    if(hide == seek){
      if(RAND() % 2)
	hide --;
      else
	seek --;
      if(hide > seek){
	printf("Conseguiu se esconder de %s.\n", p2 -> name);
	p1 -> hiding = true;
      }
      else
	printf("Mas falhou em se esconder de %s.\n", p2 -> name);
    }
  }
  else{
    int damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> hide_att_type, 0);
    int resistance = compute_weakness(p1 -> aim_att_type, p2);
    p1 -> hiding = false;
#if defined(AUTOMATIC_WILLPOWER)
    if(p1 -> willpower > 0)
      p1 -> willpower --;
    if(p2 -> willpower > 0 && !(p2 -> grappled))
      p2 -> willpower --;
#endif
    if(damage < 0){
      printf("%s atacou %s com '%s', mas errou.\nPerdeu 2 de Iniciativa.\n",
	     p1 -> name, p2 -> name, p1 -> hide_att_move);
      if(p1 -> initiative >= 11)
	add_initiative(p1, p2, -3, true);
      else
	add_initiative(p1, p2, -2, true);
      if(p2 -> control_turns){
	p2 -> control_turns --;
	if(p2 -> control_turns <= 0){
	  p2 -> control_turns = 0;
	  p1 -> grappled = false;
	  printf("%s se soltou!\n", p1 -> name);
	}
      }
    }
    else{
      p2 -> damage += damage;
      p1 -> initiative = 3;
      printf("%s atacou %s com '%s' e acertou!\n", p1 -> name, p2 -> name,
	     p1 -> hide_att_move);
      if(resistance < 0)
	printf("Foi Super-Efetivo!\n");
      else if(resistance > 0)
	printf("Não foi muito efetivo.\n");
      printf("Causou %d de dano na Vitalidade.\n", damage);
      if(p2 -> damage > p2 -> vitality)
	printf("%s MORREU!!!!!\n", p2 -> name);
      if(p2 -> control_turns){
	p2 -> control_turns -= 2;
	if(p2 -> control_turns <= 0){
	  p2 -> control_turns = 0;
	  p1 -> grappled = false;
	  printf("%s se soltou!\n", p1 -> name);
	}
      }
    }
    if(p2 -> size < LEGENDARY || p2 -> size == p1 -> size)
      p2 -> onslaugh_penalty -= 1;
    if(p2 -> friendly)
      p2 -> friendly --;
    if(p2 -> infatuated > 1)
      p2 -> infatuated --;
    p1 -> action = NONE;
  }
}

void aim_att(struct pokemon *p1, struct pokemon *p2){
  if(p1 -> aiming == 0){
    printf("%s começa a preparar %s.\n", p1 -> name, p1 -> aim_att_move);
    p1 -> aiming = 2;
  }
  else{
    int damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> aim_att_type,
					  p1 -> aim_att_ranged);
    int resistance = compute_weakness(p1 -> aim_att_type, p2);
#if defined(AUTOMATIC_WILLPOWER)
    if(p1 -> willpower > 0)
      p1 -> willpower --;
    if(p2 -> willpower > 0 && !(p2 -> grappled))
      p2 -> willpower --;
#endif
    if(damage < 0){
      printf("%s atacou %s com '%s', mas errou.\nPerdeu 2 de Iniciativa.\n",
	   p1 -> name, p2 -> name, p1 -> aim_att_move);
      if(p1 -> initiative >= 11)
	add_initiative(p1, p2, -3, true);
      else
	add_initiative(p1, p2, -2, true);
      if(p2 -> control_turns){
	p2 -> control_turns --;
	if(p2 -> control_turns <= 0){
	  p2 -> control_turns = 0;
	  p1 -> grappled = false;
	  printf("%s se soltou!\n", p1 -> name);
	}
      }
    }
    else{
      p2 -> damage += damage;
      p1 -> initiative = 3;
      printf("%s atacou %s com '%s' e acertou!\n", p1 -> name, p2 -> name,
	     p1 -> aim_att_move);
      if(resistance < 0)
	printf("Foi Super-Efetivo!\n");
      else if(resistance > 5)
	printf("%s é imune ao ataque.\n", p2 -> name);
      else if(resistance > 0)
	printf("Não foi muito efetivo.\n");
      printf("Causou %d de dano na Vitalidade.\n", damage);
      if(p2 -> damage > p2 -> vitality)
	printf("%s MORREU!!!!!\n", p2 -> name);
      if(p2 -> control_turns){
	p2 -> control_turns -= 2;
	if(p2 -> control_turns <= 0){
	  p2 -> control_turns = 0;
	  p1 -> grappled = false;
	  printf("%s se soltou!\n", p1 -> name);
	}
      }
    }
    if(p2 -> size < LEGENDARY || p2 -> size == p1 -> size)
      p2 -> onslaugh_penalty -= 1;
    if(p2 -> friendly)
      p2 -> friendly --;
    if(p2 -> infatuated > 1)
      p2 -> infatuated --;
    p1 -> action = NONE;
  }
}

void attack_and_disengage(struct pokemon *p1, struct pokemon *p2){
  int damage = simulate_decisive_attack(p1, p2, -3, 0, p1 -> att_disengage_type,
					p1 -> att_disengage_ranged);
  int resistance = compute_weakness(p1 -> att_disengage_type, p2);
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0 && !(p2 -> grappled))
    p2 -> willpower --;
#endif
  if(damage < 0){
    printf("%s atacou %s com '%s', mas errou.\nPerdeu 2 de Iniciativa.\n",
	   p1 -> name, p2 -> name, p1 -> att_disengage_move);
    if(p1 -> initiative >= 11)
      add_initiative(p1, p2, -3, true);
    else
      add_initiative(p1, p2, -2, true);
  }
  else{
    p2 -> damage += damage;
    p1 -> initiative = 3;
    printf("%s atacou %s com '%s' e acertou!\n", p1 -> name, p2 -> name,
	   p1 -> att_disengage_move);
    if(resistance < 0)
      printf("Foi Super-Efetivo!\n");
    else if(resistance > 5)
      printf("%s é imune ao ataque.\n", p2 -> name);
    else if(resistance > 0)
      printf("Não foi muito efetivo.\n");
    printf("Causou %d de dano na Vitalidade.\n", damage);
    if(p2 -> damage > p2 -> vitality)
      printf("%s MORREU!!!!!\n", p2 -> name);
  }
  if(p2 -> size < LEGENDARY || p2 -> size == p1 -> size)
    p2 -> onslaugh_penalty -= 1;
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
      p2 -> infatuated --;
  p1 -> onslaugh_penalty -= 1; // Flurry
  // Distância:
  {
    int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1) - 3);
    int defense = roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2));
    printf("%s gastou 2 de Iniciativa para se afastar.\n", p1 -> name);
    add_initiative(p1, p2, -2, true);
    if(attack == defense){
      if(RAND()%2)
	attack ++;
      else
	defense ++;
    }
    if(attack < defense)
      printf("%s correu e não deixou a distância aumentar.\n", p2 -> name);
    else{
      p2 -> distance = 2;
      printf("Foi bem-sucedido.  %s ganhou distância.\n", p1 -> name);
    }
  }
}

int attack_consequences(struct pokemon *p1, struct pokemon *p2, char *move){
  if(move == NULL) return NONE;
  if(p1 -> poison_remaining > 0 && p2 -> type1 != POISON &&
     p2 -> type2 != POISON &&
     (!strcmp(ATTACK_NAME(CROSS_POISON), move) ||
      !strcmp(ATTACK_NAME(GUNK_SHOT), move) ||
      !strcmp(ATTACK_NAME(POISON_JAB), move) ||
      !strcmp(ATTACK_NAME(POISON_STING), move) ||
      !strcmp(ATTACK_NAME(SLUDGE), move) ||
      !strcmp(ATTACK_NAME(SLUDGE_BOMB), move) ||
      !strcmp(ATTACK_NAME(SLUDGE_WAVE), move) ||
      !strcmp(ATTACK_NAME(SMOG), move))){// POISON
    return POISON;
  }
  else if(p1 -> poison_remaining > 0 && p2 -> type1 != ELECTRIC &&
	  p2 -> type2 != ELECTRIC &&
	  (!strcmp(ATTACK_NAME(DISCHARGE), move) ||
	   !strcmp(ATTACK_NAME(DRAGON_BREATH), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_PUNCH), move) ||
	   !strcmp(ATTACK_NAME(LICK), move) ||
	   !strcmp(ATTACK_NAME(NUZZLE), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_FANG), move) ||
	   !strcmp(ATTACK_NAME(SPARK), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_SHOCK), move) ||
	   !strcmp(ATTACK_NAME(THUNDERBOLT), move) ||
	   !strcmp(ATTACK_NAME(ZAP_CANNON), move) ||
	   !strcmp(ATTACK_NAME(THUNDER), move))){
    return PARALYSIS;
  }
  else if(p1 -> poison_remaining > 0 && p2 -> type1 != FIRE &&
	  p2 -> type2 != FIRE &&
	  (!strcmp(ATTACK_NAME(BLAZE_KICK), move) ||
	   !strcmp(ATTACK_NAME(EMBER), move) ||
	   !strcmp(ATTACK_NAME(FIRE_BLAST), move) ||
	   !strcmp(ATTACK_NAME(FIRE_FANG), move) ||
	   !strcmp(ATTACK_NAME(FIRE_PUNCH), move) ||
	   !strcmp(ATTACK_NAME(FLAME_WHEEL), move) ||
	   !strcmp(ATTACK_NAME(FLAMETHROWER), move) ||
	   !strcmp(ATTACK_NAME(HEAT_WAVE), move) ||
	   !strcmp(ATTACK_NAME(INFERNO), move) ||
	   !strcmp(ATTACK_NAME(LAVA_PLUME), move) ||
	   !strcmp(ATTACK_NAME(FLARE_BLITZ), move))){// Fire
    return BURN;
  }
  else if(!strcmp(ATTACK_NAME(BODY_SLAM), move) &&
	  p2 -> shape != HEAD && p2 -> shape != MULTIPLE_BODIES &&
	  p2 -> shape != HEAD_WITH_BASE && p2 -> shape != SERPENTINE &&
	  p2 -> shape != OCTOPUS && p2 -> crippling_penalty > -5 &&
	  p2 -> size <= p1 -> size && !(p2 -> shape == INSECTOID &&
					p1 -> size == p2 -> size)){
    return CRIPPLE;
  }
  return NONE;
}

void additional_consequences(struct pokemon *p1, struct pokemon *p2, char *move){
  if(move == NULL) return;
  if(p1 -> poison_remaining > 0 && p2 -> type1 != POISON &&
     p2 -> type2 != POISON &&
     (!strcmp(ATTACK_NAME(CROSS_POISON), move) ||
      !strcmp(ATTACK_NAME(GUNK_SHOT), move) ||
      !strcmp(ATTACK_NAME(POISON_JAB), move) ||
      !strcmp(ATTACK_NAME(POISON_STING), move) ||
      !strcmp(ATTACK_NAME(SLUDGE), move) ||
      !strcmp(ATTACK_NAME(SLUDGE_BOMB), move) ||
      !strcmp(ATTACK_NAME(SLUDGE_WAVE), move) ||
      !strcmp(ATTACK_NAME(SMOG), move))){// POISON
    int poison_turns = 4 - roll(GET_RESIST_POISON(p2));
    if(poison_turns < 2)
      poison_turns = 2;
    p2 -> poisoned += poison_turns;
    printf("%s está envenenado por %d rodadas!\n", p2 -> name,
	   p2 -> poisoned - 1);
    p1 -> poison_remaining --;
  }
  else if(p1 -> poison_remaining > 0 && p2 -> type1 != ELECTRIC &&
	  p2 -> type2 != ELECTRIC &&
	  (!strcmp(ATTACK_NAME(DISCHARGE), move) ||
	   !strcmp(ATTACK_NAME(DRAGON_BREATH), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_PUNCH), move) ||
	   !strcmp(ATTACK_NAME(LICK), move) ||
	   !strcmp(ATTACK_NAME(NUZZLE), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_FANG), move) ||
	   !strcmp(ATTACK_NAME(SPARK), move) ||
	   !strcmp(ATTACK_NAME(THUNDER_SHOCK), move) ||
	   !strcmp(ATTACK_NAME(THUNDERBOLT), move) ||
	   !strcmp(ATTACK_NAME(ZAP_CANNON), move) ||
	   !strcmp(ATTACK_NAME(THUNDER), move))){
    int poison_turns = 4 - roll(GET_RESIST_POISON(p2));
    if(poison_turns < 2)
      poison_turns = 2;
    p2 -> paralyzed += poison_turns;
    printf("%s está paralisado por %d rodadas!\n", p2 -> name,
	   p2 -> paralyzed - 1);
    p1 -> poison_remaining --;
  }
  else if(p1 -> poison_remaining > 0 && p2 -> type1 != FIRE &&
	  p2 -> type2 != FIRE &&
	  (!strcmp(ATTACK_NAME(BLAZE_KICK), move) ||
	   !strcmp(ATTACK_NAME(EMBER), move) ||
	   !strcmp(ATTACK_NAME(FIRE_BLAST), move) ||
	   !strcmp(ATTACK_NAME(FIRE_FANG), move) ||
	   !strcmp(ATTACK_NAME(FIRE_PUNCH), move) ||
	   !strcmp(ATTACK_NAME(FLAME_WHEEL), move) ||
	   !strcmp(ATTACK_NAME(FLAMETHROWER), move) ||
	   !strcmp(ATTACK_NAME(HEAT_WAVE), move) ||
	   !strcmp(ATTACK_NAME(INFERNO), move) ||
	   !strcmp(ATTACK_NAME(LAVA_PLUME), move) ||
	   !strcmp(ATTACK_NAME(FLARE_BLITZ), move))){// Fire
    p1 -> poison_remaining --;
    p2 -> burned = true;
    printf("%s começou a pegar fogo!\n", p2 -> name);
  }
  else if(!strcmp(ATTACK_NAME(BODY_SLAM), move) &&
	  p2 -> shape != HEAD && p2 -> shape != MULTIPLE_BODIES &&
	  p2 -> shape != HEAD_WITH_BASE && p2 -> shape != SERPENTINE &&
	  p2 -> shape != OCTOPUS && p2 -> crippling_penalty > -5 &&
	  p2 -> size <= p1 -> size && !(p2 -> shape == INSECTOID &&
					p1 -> size == p2 -> size)){
    p2 -> crippling_penalty -= p1 -> extra;
    if(p2 -> crippling_penalty < -5)
      p2 -> crippling_penalty = -5;
    if(p2 -> crippling_penalty > -5)
      printf("%s teve um membro deslocado!\n", p2 -> name);
    else if(p1 -> willpower > 0 && p2 -> shape != FISH &&
	    p2 -> shape != INSECTOID && p2 -> shape != FLYING_BUG &&
	    p2 -> shape != HEAD_WITH_ARMS){
      p1 -> willpower --;
      p2 -> broken_leg = true;
      printf("%s teve o fêmur fraturado!\n", p2 -> name);
      p2 -> prone = true;
    }
    else if(p2 -> crippling_penalty <= -5)
      printf("%s teve um membro quebrado!\n", p2 -> name);
  }
}

void decisive_attack(struct pokemon *p1, struct pokemon *p2){
  int i, resistance, damage;
  if(p1 -> control_turns > 0)
    damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type, DEFAULT);
  else
    damage = simulate_decisive_attack(p1, p2, 0, 0, NONE, DEFAULT);
  i = (p1 -> initiative - 1) / 2;
  if(i > 25) i = 25;
  while(p1 -> description[i] == NULL)
    i --;
  if(p1 -> control_turns > 0)
    resistance = compute_weakness(p1 -> grapple_type, p2);
  else
    resistance = compute_weakness(p1 -> type[i], p2);
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0 && !(p2 -> grappled))
    p2 -> willpower --;
#endif
  if(damage < 0){
    printf("%s atacou %s com '%s', mas errou.\nPerdeu 2 de Iniciativa.\n",
	   p1 -> name, p2 -> name, (p1 -> control_turns)?
	   (p1 -> grapple_move):(p1 -> description[i]));
    if(p1 -> initiative >= 11)
      add_initiative(p1, p2, -3, true);
    else
      add_initiative(p1, p2, -2, true);
    if(p2 -> control_turns){
      p2 -> control_turns --;
      if(p2 -> control_turns <= 0){
	p2 -> control_turns = 0;
	p1 -> grappled = false;
	printf("%s se soltou!\n", p1 -> name);
      }
    }
  }
  else{
    char *move;
    p2 -> damage += damage;
    p1 -> initiative = 3;
    move = ((p1 -> control_turns)?(p1 -> grapple_move):(p1 -> description[i]));
    printf("%s atacou %s com '%s' e acertou!\n", p1 -> name, p2 -> name, move);
    if(resistance < 0)
      printf("Foi Super-Efetivo!\n");
    else if(resistance > 5)
      printf("%s é imune ao ataque.\n", p2 -> name);
    else if(resistance > 0)
      printf("Não foi muito efetivo.\n");
    printf("Causou %d de dano na Vitalidade.\n", damage);
    additional_consequences(p1, p2, move);
    if(p2 -> damage > p2 -> vitality)
      printf("%s MORREU!!!!!\n", p2 -> name);
    if(p2 -> control_turns){
      p2 -> control_turns -= 2;
      if(p2 -> control_turns <= 0){
	p2 -> control_turns = 0;
	p1 -> grappled = false;
	printf("%s se soltou!\n", p1 -> name);
      }
    }
  }
  if(p2 -> size < LEGENDARY || p2 -> size == p1 -> size)
    p2 -> onslaugh_penalty -= 1;
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --;
}

// Teleport
void disengage(struct pokemon *p1, struct pokemon *p2){
  int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1));
  int defense = roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2));
  printf("%s gastou 2 de Iniciativa para usar Teleporte.\n", p1 -> name);
  add_initiative(p1, p2, -2, true);
  if(attack == defense){
    if(RAND()%2)
      attack ++;
    else
      defense ++;
  }
  if(attack < defense)
    printf("%s correu e não deixou a distância aumentar.\n", p2 -> name);
  else{
    p2 -> distance = 2;
    printf("Foi bem-sucedido.  %s ganhou distância.\n", p1 -> name);
  }
}


// Smokescreen
void smokescreen_move(struct pokemon *p1, struct pokemon *p2){
  int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1));
  int defense = roll(GET_ATTACK_DICE(p2) + GET_BONUS_ATTACK(p2));
  printf("%s gastou 2 de Iniciativa para usar %s.\n", p1 -> name,
	 p1 -> smoke_move);
  add_initiative(p1, p2, -2, true);
  p1 -> smoke_move = NULL;
  if(attack == defense){
    if(RAND()%2)
      attack ++;
    else
      defense ++;
  }
  if(attack < defense)
    printf("O golpe falhou.\n");
  else{
    p2 -> distance = 2;
    p2 -> blinded = true;
    printf("Foi bem-sucedido. %s está cego. %s ganhou distância.\n",
	   p2 -> name, p1 -> name);
  }
}

// Inspire anger
void inspire_anger(struct pokemon *p1, struct pokemon *p2){
  int resolve, result, bonus = 0;
  printf("%s está provocando raiva em %s com '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> anger_move);
  resolve = GET_RESOLVE_INT(p2);
  if(p2 -> afraid)
    resolve += 2;
  if(p2 -> friendly)
    resolve += 3;
  if(p2 -> infatuated)
    resolve += 3;
  result = roll(GET_THREATEN_DICE(p1) + bonus);
  if(p1 -> willpower > 0 && p2 -> willpower == 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve)
    printf("Não surtiu nenhum efeito.\n");
  else{
    if(p2 -> willpower > 0){
      p2 -> willpower --;
      printf("%s resistiu à provocação gastando Força de Vontade.\n", p2 -> name);
    }
    else{
      printf("%s foi provocado e está furioso.\n", p2 -> name);
      p2 -> angry = 1 + result - resolve;
      p2 -> afraid = 0;
      p2 -> friendly = 0;
      if(p2 -> infatuated)
	p2 -> infatuated = 1;
    }
  }
  p1 -> anger_move = NULL;
}

// Inspire fear
void inspire_fear(struct pokemon *p1, struct pokemon *p2){
  int resolve, result, bonus = 0;
  printf("%s tentou deixar %s com medo com '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> scare_move);
  resolve = GET_RESOLVE_INT(p2);
  //if(p1 -> appearance > resolve && p1 -> hideous)
  //  bonus = p1 -> appearance - resolve;
  if(p2 -> angry)
    resolve += 3;
  //resolve += 4; // Pokémon lutar até o fim é Princípio Definidor
  result = roll(GET_MANIPULATION_DICE(p1) + bonus);
  if(p1 -> willpower > 0 && p2 -> willpower == 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve)
    printf("Não surtiu nenhum efeito.\n");
  else{
    if(p2 -> willpower > 0){
      p2 -> willpower --;
      printf("%s resistiu à ameaça gastando Força de Vontade.\n", p2 -> name);
    }
    else{
      printf("%s ficou assustado e está com medo.\n", p2 -> name);
      p2 -> afraid = 1 + result - resolve;
      p2 -> angry = 0;
      p2 -> friendly = 0;
    }
  }
  p1 -> scare_move = NULL;
}

// Inspire fear
void inspire_friendship(struct pokemon *p1, struct pokemon *p2){
  int resolve, result, bonus = 0;
  printf("%s usou '%s'.\n", p1 -> name, p1 -> friendship_move);
  resolve = GET_RESOLVE_INT(p2);
  //if(p1 -> appearance > resolve && !(p1 -> hideous))
  //  bonus = p1 -> appearance - resolve;
  if(p2 -> angry || p2 -> afraid)
    resolve += 3;
  //resolve += 4; // Pokémon lutar até o fim é Princípio Definidor
  if(p1 -> friendship_move == fake_tears)
    result = roll(GET_MANIPULATION_DICE(p1) + bonus);
  else
    result = roll(GET_CHARISMA_DICE(p1) + bonus);
  if(p1 -> willpower > 0 && p2 -> willpower == 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve)
    printf("Não surtiu nenhum efeito.\n");
  else{
    if(p2 -> willpower > 0){
      p2 -> willpower --;
      printf("%s resistiu gastando Força de Vontade.\n", p2 -> name);
    }
    else{
      printf("%s ficou preocupado em machucar %s.\n", p2 -> name, p1 -> name);
      p2 -> friendly = 1 + result - resolve;
      p2 -> angry = 0;
      p2 -> afraid = 0;
    }
  }
  p1 -> friendship_move = p1 -> friendship2;
  p1 -> friendship2 = p1 -> friendship3;
  p1 -> friendship3 = NULL;
}

// Inspire love
void inspire_love(struct pokemon *p1, struct pokemon *p2){
  int resolve, result, bonus = 0;
  printf("%s usou '%s'.\n", p1 -> name, p1 -> flirt_move);
  if(p1 -> flirt_move != p1 -> flirt2 && RAND() % 2){
    printf("%s não sente nenhuma atração por %s.\n", p2 -> name, p1 -> name);
    p1 -> flirt_move = NULL;
    p1 -> flirt2 = NULL;
    p1 -> seduce_move = NULL;
    return;
  }
  resolve = GET_RESOLVE_INT(p2);
  //if(p1 -> appearance > resolve && !(p1 -> hideous))
  //  bonus = p1 -> appearance - resolve;
  if(p2 -> angry || p2 -> afraid)
    resolve += 3;
  else if(p2 -> friendly)
    resolve -= 2;
  //resolve += 4; // Pokémon lutar até o fim é Princípio Definidor
  result = roll(GET_CHARISMA_DICE(p1) + bonus);
  if(p1 -> willpower > 0 && p2 -> willpower == 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve)
    printf("Não surtiu nenhum efeito.\n");
  else{
    if(p2 -> willpower > 0){
      p2 -> willpower --;
      printf("%s resistiu gastando Força de Vontade.\n", p2 -> name);
    }
    else{
      printf("%s está apaixonado por %s.\n", p2 -> name, p1 -> name);
      p2 -> infatuated = 1 + result - resolve;
      p2 -> angry = 0;
      p2 -> afraid = 0;
    }
  }
  if(p1 -> flirt_move == p1 -> flirt2)
    p1 -> flirt_move = p1 -> flirt2 = NULL;
  else
    p1 -> flirt_move = p1 -> flirt2;
}


// Threaten
void threaten(struct pokemon *p1, struct pokemon *p2){
  int resolve, result, bonus = 0;
  printf("%s ameaçou %s usando %s.\n", p1 -> name, p2 -> name,
	 p1 -> threaten_move);
  resolve = GET_RESOLVE_INT(p2) + ((p2 -> afraid)?(-2):(0));
  resolve += 4; // Pokémon lutar até o fim é Princípio Definidor
  if(p2 -> willpower > 0){
    p2 -> willpower --;
    resolve ++;
  }
  if(p1 -> appearance > (resolve - 4) && p1 -> hideous)
    bonus = p1 -> appearance - resolve + 4;
  if(p1 -> hideous)
    result = roll(GET_THREATEN_DICE(p1) + GET_SOCIAL_BONUS(p1, p2));
  else
    result = roll(GET_THREATEN_DICE(p1));
  if(p2 -> afraid)
    resolve -= 2;
  if(resolve < 0)
    resolve = 0;
  if(result < 0)
    result = 0;
  if(p1 -> willpower > 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve){
    printf("Não surtiu nenhum efeito.\n");
    if(p2 -> afraid && p2 -> resisted_threat[0] == false)
      p2 -> resisted_threat[0] = true;
    else
      p2 -> resisted_threat[-(p2-> penalty[p2 -> damage])/2] = true;
  }
  else if(p2 -> willpower > 0 && p2 -> angry > 0){
    printf("%s gastou 1 de Força de Vontade e resistiu apelando à sua raiva.\n",
	   p2 -> name);
    p2 -> willpower --;
  }
  else{
    printf("%s se apavorou, desistiu da luta e se rendeu!!!!!\n", p2 -> name);
    p2 -> forfeit = true;
  }
  p2 -> friendly = 0;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --;
}

// Seduce
void seduce(struct pokemon *p1, struct pokemon *p2){
  int resolve, result;
  printf("%s tentou conquistar %s usando %s.\n", p1 -> name, p2 -> name,
	 p1 -> seduce_move);
  resolve = GET_RESOLVE_INT(p2) + ((p2 -> afraid)?(-2):(0));
  resolve += 4; // Pokémon lutar até o fim é Princípio Definidor
  if(p2 -> willpower > 0){
    p2 -> willpower --;
    resolve ++;
  }
  if(!(p1 -> hideous))
    result = roll(GET_THREATEN_DICE(p1) + GET_SOCIAL_BONUS(p1, p2));
  else
    result = roll(GET_THREATEN_DICE(p1));
  if(result < 0)
    result = 0;
  if(p1 -> willpower > 0){
    p1 -> willpower --;
    result ++;
  }
  if(result < resolve){
    printf("Não surtiu nenhum efeito.\n");
    p1 -> seduce_move = NULL;
  }
  else if(p2 -> willpower > 0 && (p2 -> angry > 0 || p2 -> afraid > 0)){
    printf("%s gastou 1 de Força de Vontade e resistiu apelando %s.\n",
	   p2 -> name, (p2 -> angry)?("à sua raiva"):("ao seu medo"));
    p1 -> seduce_move = NULL;
  }
  else{
    printf("%s está profundamente apaixonado e se rendeu!!!!!\n", p2 -> name);
    p2 -> forfeit = true;
  }
  if(p2 -> afraid > 1)
    p2 -> afraid --;
}

void transform(struct pokemon *p1, struct pokemon *p2){
  int i = 1, j = 1, k, to_increment = 3;
  int sum, choice;
  bool done = false;
  p1 -> can_transform = false;
  strcpy(p1 -> desc, p2 -> desc);
  while(p2 -> description[i] != NULL){
    for(k = j + 1; k < 26; k ++){
      p1 -> description[k] = p1 -> description[k - 1];
      p1 -> ranged[k] = p1 -> ranged[k - 1];
      p1 -> type[k] = p1 -> type[k - 1];
    }
    p1 -> description[j] = p2 -> description[i];
    p1 -> ranged[j] = p2 -> ranged[i];
    p1 -> type[j] = p2 -> type[i];
    j ++;
    if(p1 -> description[j] != NULL)
      j ++;
    i ++;
  }
  sum = (p2 -> strength > p1 -> strength) + (p2 -> dexterity > p1 -> dexterity) +
    (p2 -> stamina > p1 -> stamina) + (p2 -> charisma > p1 -> charisma) +
    (p2 -> manipulation > p1 -> manipulation) + (p2 -> wits > p1 -> wits) +
    (p2 -> appearance > p1 -> appearance) +
    (p2 -> perception > p1 -> perception) +
    (p2 -> inteligence > p1 -> inteligence);
  while(to_increment > 0 && sum > 0){
    choice = RAND() % 9;
    switch(choice){
    case 0:
      if(p1 -> strength < p2 -> strength){
	if(p2 -> strength >= p1 -> strength + 2){
	  p1 -> strength += 2;
	}
	else{
	  p1 -> strength += 1;
	}
	to_increment --;
	sum --;
      }
      break;
    case 1:
      if(p1 -> dexterity < p2 -> dexterity){
	if(p2 -> dexterity >= p1 -> dexterity + 2){
	  p1 -> dexterity += 2;
	}
	else{
	  p1 -> dexterity ++;
	}  
	to_increment --;
	sum --;
      }
      break;
    case 2:
      if(p1 -> stamina < p2 -> stamina){
	if(p2 -> stamina >= p1 -> stamina + 2){
	  p1 -> stamina += 2;
	}
	else{
	  p1 -> stamina ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 3:
      if(p1 -> charisma < p2 -> charisma){
	if(p2 -> charisma >= p1 -> charisma + 2){
	  p1 -> charisma += 2;
	}
	else{
	  p1 -> charisma ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 4:
      if(p1 -> manipulation < p2 -> manipulation){
	if(p2 -> manipulation >= p1 -> manipulation + 2){
	  p1 -> manipulation += 2;
	}
	else{
	  p1 -> manipulation ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 5:
      if(p1 -> appearance < p2 -> appearance){
	if(p2 -> appearance >= p1 -> appearance + 2){
	  p1 -> appearance += 2;
	}
	else{
	  p1 -> appearance ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 6:
      if(p1 -> inteligence < p2 -> inteligence){
	if(p2 -> inteligence >= p1 -> inteligence + 2){
	  p1 -> inteligence += 2;
	}
	else{
	  p1 -> inteligence ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 7:
      if(p1 -> wits < p2 -> wits){
	if(p2 -> wits >= p1 -> wits + 2){
	  p1 -> wits += 2;
	}
	else{
	  p1 -> wits ++;
	}
	to_increment --;
	sum --;
      }
      break;
    case 8:
      if(p1 -> perception < p2 -> perception){
	if(p2 -> perception >= p1 -> perception + 2)
	  p1 -> perception += 2;
	else
	  p1 -> perception ++;
	to_increment --;
	sum --;
      }
      break;
    }
  }
  if(to_increment && p2 -> essence > p1 -> essence)
    p1 -> essence ++;
  p1 -> shape = p2 -> shape;
  p1 -> size = p2 -> size;
  p1 -> hideous = p2 -> hideous;
  while(!done){
    choice = RAND() % 22;
    switch(choice){
    case 0:
      p1 -> type1 = p2 -> type1;
      done = true;
      break;
    case 1:
      if(p2 -> aim_move != NULL){
	p1 -> aim_move = p2 -> aim_move;
	done = true;
      }
      break;
    case 2:
      if(p2 -> full_defense_move != NULL){
	p1 -> full_defense_move = p2 -> full_defense_move;
	done = true;
      }
      break;
    case 3:
      if(p2 -> threaten_move != NULL){
	p1 -> threaten_move = p2 -> threaten_move;
	done = true;
      }
      break;
    case 4:
      if(p2 -> poison_move != NULL){
	p1 -> poison_move = p2 -> poison_move;
	done = true;
      }
      break;
    case 5:
      if(p2 -> sleep_move != NULL){
	p1 -> sleep_move = p2 -> sleep_move;
	done = true;
      }
      break;
    case 6:
      if(p2 -> other_senses == true){
	p1 -> other_senses = true;
	done = true;
      }
      break;
    case 7:
      if(p2 -> smoke_move != NULL){
	p1 -> smoke_move = p2 -> smoke_move;
	done = true;
      }
      break;
    case 8:
      if(p2 -> can_teleport){
	p1 -> can_teleport = true;
	done = true;
      }
      break;
    case 9:
      if(p2 -> mental_attack == true){
	p1 -> mental_attack = true;
	done = true;
      }
      break;
    case 10:
      if(p2 -> att_disengage_move != NULL){
	p1 -> att_disengage_move = p2 -> att_disengage_move;
	p1 -> att_disengage_type = p2 -> att_disengage_type;
	p1 -> att_disengage_ranged = p2 -> att_disengage_ranged;
	done = true;
      }
      break;
    case 11:
      if(p2 -> aim_att_move != NULL){
	p1 -> aim_att_move = p2 -> aim_att_move;
	p1 -> aim_att_type = p2 -> aim_att_type;
	p1 -> aim_att_ranged = p2 -> aim_att_ranged;
	done = true;
      }
      break;
    case 12:
      if(p2 -> hide_att_move != NULL){
	p1 -> hide_att_move = p2 -> hide_att_move;
	p1 -> hide_att_type = p2 -> hide_att_type;
	done = true;
      }
      break;
    case 13:
      if(p2 -> take_cover_move != NULL){
	p1 -> take_cover_move = p2 -> take_cover_move;
	done = true;
      }
      break;
    case 14:
      if(p2 -> dif_terrain_move != NULL){
	p1 -> dif_terrain_move = p2 -> dif_terrain_move;
	done = true;
      }
      break;
    case 15:
      if(p2 -> grapple_move != NULL){
	p1 -> grapple_move = p2 -> grapple_move;
	p1 -> grapple_type = p2 -> grapple_type;
	p1 -> grapple_category = p2 -> grapple_category;
	done = true;
      }
      break;
    case 16:
      if(p2 -> slam_move != NULL){
	p1 -> slam_move = p2 -> slam_move;
	p1 -> slam_type = p2 -> slam_type;
	done = true;
      }
      break;
    case 17:
      if(p2 -> ambush_move != NULL){
	p1 -> ambush_move = p2 -> ambush_move;
	p1 -> ambush_type = p2 -> ambush_type;
	p1 -> ambush_attack = p2 -> ambush_attack;
	done = true;
      }
      break;
    case 18:
      if(p2 -> scare_move != NULL){
	p1 -> scare_move = p2 -> scare_move;
	done = true;
      }
      break;
    case 19:
      if(p2 -> anger_move != NULL){
	p1 -> anger_move = p2 -> anger_move;
	done = true;
      }
      break;
    case 20:
      if(p2 -> friendship_move != NULL){
	p1 -> friendship_move = p2 -> friendship_move;
	done = true;
      }
      break;
    case 21:
      if(p2 -> can_transform){
	p1 -> can_transform = true;
	done = true;
      }
      break;      
    }
  }
  printf("%s está mudando sua forma para a de %s.\n", p1 -> name, p2 -> name);
  p1 -> onslaugh_penalty -= 2; // Ação miscelânea
}

void throw_paralysis(struct pokemon *p1, struct pokemon *p2){
  int damage, type = NONE;
  printf("%s tentou paralisar %s usando '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> paralysis_move);
  p1 -> poison_remaining --;
  if(p1 -> paralysis_move == thunder_wave)
    type = ELECTRIC;
  else if(p1 -> paralysis_move == stun_spore)
    type = GRASS;
  damage = simulate_decisive_attack(p1, p2, 0, 0, type, 1); // perde 4 ao falhar
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0)
    p2 -> willpower --;
  if(damage < 3){
    printf("Mas o ataque falhou.\n");
    add_initiative(p1, p2, -4, true);
  }
  else{
    int poison_turns = 4 - roll(GET_RESIST_POISON(p2));
    if(poison_turns < 2)
      poison_turns = 2;
    p2 -> paralyzed += poison_turns;
    printf("Acertou e %s está paralisado por %d rodadas!\n", p2 -> name,
	   p2 -> paralyzed - 1);
  }
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --;
}

void throw_poison(struct pokemon *p1, struct pokemon *p2){
  int damage, type = POISON;
  printf("%s lançou veneno em %s usando '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> poison_move);
  p1 -> poison_remaining --;
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0)
    p2 -> willpower --;
  if(damage < 3){
    printf("Mas o ataque falhou.\n");
    add_initiative(p1, p2, -4, true);
  }
  else{
    int poison_turns = 4 - roll(GET_RESIST_POISON(p2));
    if(poison_turns < 2)
      poison_turns = 2;
    p2 -> poisoned += poison_turns;
    printf("Acertou e %s está envenenado por %d rodadas!\n", p2 -> name,
	   p2 -> poisoned - 1);
  }
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --; 
}

void throw_burn(struct pokemon *p1, struct pokemon *p2){
  int damage, type = FIRE;
  printf("%s tacou fogo em %s usando '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> burn_move);
  damage = simulate_decisive_attack(p1, p2, 0, 0, FIRE, 1);
  p1 -> poison_remaining --;
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0)
    p2 -> willpower --;
  if(damage < 3){
    printf("Mas o ataque falhou.\n");
    add_initiative(p1, p2, -4, true);
  }
  else{
    p2 -> burned = true;
    printf("Acertou e %s está pegando fogo!\n", p2 -> name);
  }
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --; 
}

void end_burn(struct pokemon *p){
  p -> onslaugh_penalty -= 2; // Ação miscelânea
  p -> burned = false;
  if(p -> type1 == WATER || p -> type2 == WATER){
    printf("%s apagou o fogo jogando água nele.\n", p -> name);
  }
  else{
    printf("%s apagou o fogo se jogando no chão.\n", p -> name);
    if(p -> shape != HEAD && p -> shape != FISH && p -> shape != FLYING_BUG &&
       p -> shape != MULTIPLE_BODIES && p -> shape != OCTOPUS &&
       p -> shape != HEAD_WITH_BASE && p -> shape != BIRD &&
       p -> shape != HEAD_WITH_ARMS && p -> shape != INSECTOID &&
       p -> shape != QUADRUPED)
      p -> prone = true;
  }
}

void make_sleep(struct pokemon *p1, struct pokemon *p2){
  int success;
  if(p1 -> making_sleep == false){
    p1 -> making_sleep = true;
    p1 -> willpower -= 2;
    if(p1 -> sleep_move == sleep_powder)
      printf("%s começou a soltar Pó do Sono. ", p1 -> name);
    else if(p1 -> sleep_move == hypnosis)
      printf("%s começou a hipnotizar %s. ", p1 -> name, p2 -> name);
    else if(p1 -> sleep_move == sing)
      printf("%s começou canção de ninar para %s.", p1 -> name, p2 -> name);
    else if(p1 -> sleep_move == yawn)
      printf("%s começou a bocejar. ", p1 -> name);
    printf("Gastou 2 de Força de Vontade.\n");
  }
  else{
    if(p1 -> sleep_move == sleep_powder)
      printf("%s continuou a soltar Pó do Sono.\n", p1 -> name);
    else if(p1 -> sleep_move == hypnosis)
      printf("%s continuou a hipnotizar %s.\n", p1 -> name, p2 -> name);
    else if(p1 -> sleep_move == hypnosis)
      printf("%s continuou sua canção de ninar.\n", p1 -> name);
    else if(p1 -> sleep_move == yawn)
      printf("%s continuou a bocejar e espreguiçar.\n", p1 -> name);
  }
  p1 -> motes += roll(GET_MAGIC_DICE(p1));
  if(p1 -> motes <= 0){
    p1 -> motes = 0;
    if(p1 -> sleep_move == sleep_powder)
      printf("Mas o pó se dispersou com o vento e se perdeu.\n");
    else if(p1 -> sleep_move == hypnosis)
      printf("Mas a concentração da hipnose se perdeu.\n");
    else if(p1 -> sleep_move == sing)
      printf("Mas acabou esquecendo a letra da música.\n");
    else if(p1 -> sleep_move == yawn)
      printf("Mas isso não está mais afetando %s.\n", p2 -> name);
    p1 -> making_sleep = false;
    p1 -> sleep_move = NULL;
  }
  else{
    if((p1 -> motes >= 7 && p1 -> initiative > 0) ||
       (p1 -> motes >= 10 && p1 -> initiative <= 0)){
      if(p1 -> sleep_move == sleep_powder)
	printf("Pó do Sono está fazendo efeito.\n");
      else if(p1 -> sleep_move == hypnosis)
	printf("A hipnose está fazendo efeito.\n");
      else if(p1 -> sleep_move == sing)
	printf("A canção de ninar está fazendo efeito.\n");
      else if(p1 -> sleep_move == yawn)
	printf("O sono contagioso acabou pegando em %s.\n", p2 -> name);
      if(p1 -> initiative > 0){
	printf("%s recuperou 1 de Força de Vontade.\n", p1 -> name);
	p1 -> willpower ++;
      }
      p2 -> in_the_mist = 2;
      p1 -> making_sleep = false;
      p1 -> sleep_move = NULL;
    }
    else{
      float total = ((p1 -> initiative > 0)?(7.0):(10.0));
      if(p1 -> sleep_move == sleep_powder)
	printf("Pó do Sono começa a se acumular. Há %d%% do necessário.\n",
	       (int) (100 * (p1 -> motes / total)));
      else if(p1 -> sleep_move == hypnosis)
	printf("%s está se concentrando. Há %d%% de concentração.\n",
	       p1 -> name,
	       (int) (100 * (p1 -> motes / total)));
      else if(p1 -> sleep_move == sing)
	printf("%s está cantando. Terminou %d%% da música.\n",
	       p1 -> name,
	       (int) (100 * (p1 -> motes / total)));
      else if(p1 -> sleep_move == yawn)
      printf("%s bocejou também e está %d%% afetado pelo sono.\n",
	     p2 -> name,
	     (int) (100 * (p1 -> motes / total)));
    }
  }
}

int simulate_clash_attack(struct pokemon *p1, struct pokemon *p2,
			   int attack1, int attack2, int type1, int type2,
			   int bonus1, int bonus2, bool *legendary){
  int roll1, roll2, damage;
  struct pokemon *winner, *loser;
  if(attack1 == WITHERING_ATTACK || attack1 == CLASH_WITHERING)
    roll1 = roll(GET_WITHERING_DICE(p1) + GET_BONUS_ATTACK(p1));
  else
    roll1 = roll(GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1));
  if(attack2 == WITHERING_ATTACK || attack2 == CLASH_WITHERING)
    roll2 = roll(GET_WITHERING_DICE(p2) + GET_BONUS_ATTACK(p2));
  else
    roll2 = roll(GET_ATTACK_DICE(p2) + GET_BONUS_ATTACK(p2));
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    attack1 ++;
  if(p2 -> willpower > 0)
    attack2 ++;
#endif
  if(attack1 > attack2 || (attack1 == attack2 && RAND() % 2)){
    // 1 ganhou
    if(attack1 == WITHERING_ATTACK){
      damage = ROLL_ATTACK(p1, p2, attack1 - attack2, legendary);
      if(damage <= 0)
	damage = 1;
      else
	damage ++;
    }
    else{
      int resistance = compute_weakness(type1, p2);
      damage = roll_no_double(p1 -> initiative) - resistance;
      if(resistance > 2 || damage <= 0)
	damage = 1;
      else
	damage += 3;
    }
  }
  else{
    // 2 ganhou
    if(attack1 == WITHERING_ATTACK){
      damage = ROLL_ATTACK(p1, p2, attack1 - attack2, legendary);
      if(damage <= 0)
	damage = -1;
      else
	damage = -damage - 3;
    }
    else{
      int resistance = compute_weakness(type2, p1);
      damage = roll_no_double(p2 -> initiative) - resistance;
      if(resistance > 2 || damage <= 0)
	damage = -1;
      else
	damage = - damage - 1;
    }
  }
  return damage;
}

void clash_attack(struct pokemon *p1, struct pokemon *p2){
  int attack1, attack2, category1, category2, i, i1, i2, extra_success,
    winner_category, initial_initiative;
  int damage, bonus1 = 0, bonus2 = 0, type1 =NONE, type2 = NONE;
  struct pokemon *winner = NULL, *loser = NULL;
  char move1[36], move2[36];
  bool decisive, legendary;
  move1[0] = move2[0] = '\0';
  if(p1 -> action == ATT_DISENGAGE){
    bonus1 = -3;
    type1 = p1 -> att_disengage_type;
    strcpy(move1, p1 -> att_disengage_move);
  }
  else if(p1 -> action == AIM_ATT){
    strcpy(move1, p1 -> aim_att_move);
    type1 = p1 -> aim_att_type;
  }
  if(p2 -> action == ATT_DISENGAGE){
    bonus2 = -3;
    type2 = p2 -> att_disengage_type;
    strcpy(move2, p2 -> att_disengage_move);
  }
  else if(p2 -> action == AIM_ATT){
    strcpy(move2, p2 -> aim_att_move);
    type2 = p2 -> aim_att_type;
  }
  if(p1 -> action == CLASH_WITHERING || p1 -> action == WITHERING_ATTACK){
    p1 -> action = WITHERING_ATTACK;
    switch(GET_ATTACK_TYPE(p1, p2)){
    case SPECIAL:
      strcpy(move1, "Ataque Especial");
      break;
    case PHYSICAL:
      strcpy(move1, "Ataque Físico");
      break;
    default:
      strcpy(move1, "Ataque Mental");
      break;
    }
  }
  else
    p1 -> action = DECISIVE_ATTACK;
  if(p2 -> action == CLASH_WITHERING || p2 -> action == WITHERING_ATTACK){
    p2 -> action = WITHERING_ATTACK;
    switch(GET_ATTACK_TYPE(p1, p2)){
    case SPECIAL:
      strcpy(move2, "Ataque Especial");
      break;
    case PHYSICAL:
      strcpy(move2, "Ataque Físico");
      break;
    default:
      strcpy(move2, "Ataque Mental");
      break;
    }
  }
  else
    p2 -> action = DECISIVE_ATTACK;
  if(type1 == NONE && p1 -> initiative > 0){
    i1 = (p1 -> initiative - 1) / 2;
    if(i1 > 25) i1 = 25;
    while(p1 -> description[i1] == NULL)
      i1 --;
    type1 = p1 -> type[i1];
    if(move1[0] == '\0')
      strcpy(move1, p1 -> description[i1]);
  }
  if(type2 == NONE && p2 -> initiative > 0){
    i2 = (p2 -> initiative - 1) / 2;
    if(i2 > 25) i2 = 25;
    while(p2 -> description[i2] == NULL)
      i2 --;
    type2 = p2 -> type[i2];
    if(move2[0] == '\0')
      strcpy(move2, p2 -> description[i2]);
  }
  damage = simulate_clash_attack(p1, p2, p1 -> action, p2 -> action,
				 type1, type2,  bonus1, bonus2, &legendary);
  printf("%s e %s se atacaram simultaneamente!\n", p1 -> name, p2 -> name);
  printf("%s atacou com %s ", p1 -> name, move1);
  printf("e %s atacou com %s!\n", p2 -> name, move2);
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0)
    p2 -> willpower --;
#endif
  if(damage > 0){
    winner = p1; loser = p2;
    add_initiative(p2, p1, -2, false);
    if(p1 -> action == WITHERING_ATTACK || p1 -> action == CLASH_WITHERING){
      printf("%s venceu e roubou %d de Iniciativa de %s!\n",
	     p1 -> name, damage, p2 -> name);
      steal_initiative(p1, p2, damage, legendary);
    }
    else{
      printf("%s venceu e causou %d de dano!!\n", p1 -> name, damage);
      if(compute_weakness(type1, p2) < 0)
	printf("Foi super-efetivo!\n");
      else if(compute_weakness(type1, p2) > 0)
	printf("Não foi muito efetivo.\n");
      p2 -> damage += damage;
      p1 -> initiative = 3;
      additional_consequences(p1, p2, move1);
    }
  }
  else if(damage < 0){
    winner = p2; loser = p1;
    damage *= -1;
    add_initiative(p1, p2, -2, false);
    if(p2 -> action == WITHERING_ATTACK || p2 -> action == CLASH_WITHERING){
      printf("%s venceu e roubou %d de Iniciativa de %s!\n",
	     p2 -> name, damage, p1 -> name);
      steal_initiative(p2, p1, damage, legendary);
    }
    else{
      printf("%s venceu e causou %d de dano!!\n", p2 -> name, damage);
      if(compute_weakness(type2, p1) < 0)
	printf("Foi super-efetivo!\n");
      else if(compute_weakness(type2, p1) > 0)
	printf("Não foi muito efetivo.\n");
      p1 -> damage += damage;
      p2 -> initiative = 3;
      additional_consequences(p2, p1, move2);
    }
  }
  if(winner -> control_turns){
    winner -> control_turns --;
    if(winner -> control_turns <= 0){
      winner -> control_turns = 0;
      loser -> grappled = false;
      printf("%s se soltou!\n", loser -> name);
    }
  }
  if(loser -> control_turns){
    loser -> control_turns --;
    if(loser -> control_turns <= 0){
      loser -> control_turns = 0;
      winner -> grappled = false;
      printf("%s se soltou!\n", winner -> name);
    }
  }
  if(loser -> damage > loser -> vitality)
    printf("%s MORREU!!!!!\n", loser -> name);
  loser -> onslaugh_penalty -= 2;
  if(loser -> friendly)
    loser -> friendly --;
  if(winner -> friendly)
    loser -> friendly --;
  if(winner -> infatuated > 1)
    winner -> infatuated --;
  if(loser -> infatuated > 1)
    loser -> infatuated --;
  if(p1 -> att_disengage_move != NULL &&
     !strcmp(move1, p1 -> att_disengage_move)){
    int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1) - 3);
    int defense = roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2));
    printf("%s gastou 2 de Iniciativa para se afastar.\n", p1 -> name);
    add_initiative(p1, p2, -2, true);
    if(attack == defense){
      if(RAND()%2)
	attack ++;
      else
	defense ++;
    }
    if(attack < defense)
      printf("%s correu e não deixou a distância aumentar.\n", p2 -> name);
    else{
      p2 -> distance = 2;
      printf("Foi bem-sucedido.  %s ganhou distância.\n", p1 -> name);
    }
  }
  if(p2 -> att_disengage_move != NULL &&
     !strcmp(move2, p2 -> att_disengage_move)){
    int attack = roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2) - 3);
    int defense = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1));
    printf("%s gastou 2 de Iniciativa para se afastar.\n", p2 -> name);
    add_initiative(p2, p1, -2, true);
    if(attack == defense){
      if(RAND()%2)
	attack ++;
      else
	defense ++;
    }
    if(attack < defense)
      printf("%s correu e não deixou a distância aumentar.\n", p1 -> name);
    else{
      p1 -> distance = 2;
      printf("Foi bem-sucedido.  %s ganhou distância.\n", p2 -> name);
    }
  }
}

int simulate_withering_attack(struct pokemon *p1, struct pokemon *p2,
			      int extra_attack, int extra_def, bool *legendary){
  int damage, i, attack, defense, attack_bonus = 0, defense_bonus = 0;
  int category;
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    attack_bonus ++;
  if(p2 -> willpower > 0)
    defense_bonus ++;
#endif
  attack = roll(GET_WITHERING_DICE(p1) + GET_BONUS_ATTACK(p1) + extra_attack);
  defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1) + extra_def;
  attack += attack_bonus;
  defense += defense_bonus;
  if(p2 -> grappled)
    defense = 0;
  if(p1 -> covered > 2 && GET_ATTACK_TYPE(p1, p2) == PHYSICAL)
    defense += 2;
  else if(GET_ATTACK_TYPE(p1, p2) == PHYSICAL)
    defense += p1 -> covered;
  if(attack < defense)
    return -1;
  else{
    damage = ROLL_ATTACK(p1, p2, attack - defense, legendary);
    if(damage < 0)
      damage = 0;
    return damage;
  }
}

void withering_attack(struct pokemon *p1, struct pokemon *p2){
  bool legendary;
  int damage = simulate_withering_attack(p1, p2, 0, 0, &legendary);
  int category;
#if defined(AUTOMATIC_WILLPOWER)
  if(p1 -> willpower > 0)
    p1 -> willpower --;
  if(p2 -> willpower > 0 && !(p2 -> grappled))
    p2 -> willpower --;
#endif
  category = ((p1 -> strength - p2 -> stamina > p1 -> inteligence - p2 -> wits)?
	      ((p1 -> mental_attack && p1 -> inteligence  > p1 -> strength)?
	       (MENTAL):(PHYSICAL)):
	      ((p1 -> mental_attack && p2 -> stamina  < p2 -> wits)?
	       (MENTAL):(SPECIAL)));
  if(damage < 0){
    printf("%s usou ataque %s em %s, mas errou.\n", p1 -> name,
	   (category == PHYSICAL)?"físico":((category == SPECIAL)?
					    "especial":"mental"), p2 -> name);
    if(p2 -> control_turns){
      p2 -> control_turns --;
      if(p2 -> control_turns <= 0){
	p2 -> control_turns = 0;
	p1 -> grappled = false;
	printf("%s se soltou!\n", p1 -> name);
      }
    }
  }
  else{
    printf("%s usou ataque %s em %s e acertou!\n", p1 -> name,
	   (category == PHYSICAL)?"físico":((category == SPECIAL)?
					    ("especial"):("mental")), p2 -> name);
    printf("Causou %d de dano na Iniciativa%s.\n", damage,
	   ((p2 -> initiative <= 0 && p2 -> initiative + damage > 0)?
	    (" e a quebrou"):("")));
    if(p1 -> size == LEGENDARY)
      legendary = true;
    steal_initiative(p1, p2, damage, legendary);
    printf("\n");
    if(p2 -> control_turns){
      p2 -> control_turns -= 2;
      if(p2 -> control_turns <= 0){
	p2 -> control_turns = 0;
	p1 -> grappled = false;
	printf("%s se soltou!\n", p1 -> name);
      }
    }
  }
  if(p2 -> size < LEGENDARY || p1 -> size == p2 -> size)
    p2 -> onslaugh_penalty -= 1;
  if(p2 -> friendly)
    p2 -> friendly --;
  if(p2 -> infatuated > 1)
    p2 -> infatuated --;
}

void take_cover(struct pokemon *p1){
  int result = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1));
  printf("%s buscou cobertura atrás de %s.\n", p1 -> name, p1 -> take_cover_move);
  if(result <= 0)
    printf("Mas não conseguiu ir atrás da proteção a tempo.\n");
  else if(result == 1){
    printf("Se protegeu parcialmente por %s.\n", p1 -> take_cover_move);
    p1 -> covered = 1;
    p1 -> cover_duration = 4;
  }
  else if(result == 2){
    printf("Se protegeu quase totalmente por %s.\n", p1 -> take_cover_move);
    p1 -> covered = 2;
    p1 -> cover_duration = 4;
  }
  else{
    printf("Se protegeu totalmente por %s.\n", p1 -> take_cover_move);
    p1 -> covered = 3;
    p1 -> cover_duration = 4;
  }
}

void ambush(struct pokemon *p1, struct pokemon *p2){
  if(p1 -> ambush_attack == WITHERING_ATTACK){
    int resistance = compute_weakness(p1 -> ambush_type, p2);
    int attack = roll(GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1));
    int damage;
    bool legendary = false;
    attack -= 3 * resistance;
    attack += p1 -> strength - p2 -> stamina;
    if(attack >= 10)
      legendary = true;
    damage = roll1(attack);
    if(damage < 0)
      damage = 0;
    printf("%s usou '%s' em %s e acertou!\n", p1 -> name,
	   p1 -> ambush_move, p2 -> name);
    if(resistance < 0)
      printf("Não foi muito efetivo.\n");
    else if(resistance > 0)
      printf("Foi super-efetivo!\n");
    printf("Causou %d de dano na Iniciativa%s.\n", damage,
	   ((p2 -> initiative <= 0 && p2 -> initiative + damage > 0)?
	    (" e a quebrou"):("")));
    if(p1 -> size == LEGENDARY)
      legendary = true;
    steal_initiative(p1, p2, damage, legendary);
    if(p2 -> size < LEGENDARY || p1 -> size == p2 -> size)
      p2 -> onslaugh_penalty -= 1;
    if(p2 -> friendly)
      p2 -> friendly --;
    if(p2 -> infatuated > 1)
      p2 -> infatuated --;
  }
}

struct pokemon *escolhe_pokemon(int team){
  int acao;
  printf("0: Aleatório\n");
  printf("1: Bulbasaur\t2: Ivysaur\t3:Venusaur\t4: Charmander\t5: Charmeleon\n");
  printf("6: Charizard\t7: Squirtle\t8: Wartortle\t9: Blastoise\t10: Caterpie\n");
  printf("11: Metapod\t12: Butterfree\t13: Weedle\t14: Kakuna\t15: Beedrill\n");
  printf("16: Pidgey\t17: Pidgeotto\t18: Pidgeot\t19: Rattata\t20: Raticate\n");
  printf("21: Spearow\t22: Fearow\t23: Ekans\t24: Arbok\t25: Pikachu\n");
  printf("26: Raichu\t27: Sandshrew\t28: Sandslash\t29: Nidoran♀\t"
	 "30: Nidorina\n");
  printf("31: Nidoqueen\t32: Nidoran♂\t33: Nidorino\t34: Nidoking\t"
	 "35: Clefairy\n");
  printf("36: Clefable\t37: Vulpix\t38: Ninetales\t39: Jigglypuff\t "
	 "40: Wigglytuff\n");
  printf("41: Zubat\t42: Golbat\t43: Oddish\t44: Gloom\t45: Vileplume\n");
  printf("46: Paras\t47: Parasect\t48: Venonat\t49: Venomoth\t50: Digllet\n");
  printf("51: Dugtrio\t52: Meowth\t53: Persian\t54: Psyduck\t55: Golduck\n");
  printf("56: Mankey\t57: Primeape\t58: Growlithe\t59: Arcanine\t60: Poliwag\n");
  printf("61: Poliwhirl\t62: Poliwrath\t63: Abra\t64: Kadabra\t65: Alakazam\n");
  printf("66: Machop\t67: Machoke\t68: Machamp\t69: Bellsprout\t"
	 "70: Weepinbell\n");
  printf("71: Victreebel\t72: Tentacool\t73: Tentacruel\t74: Geodude\t"
	 "75: Graveler\n");
  printf("76: Golem\t77: Ponyta\t78: Rapidash\t79: Slowpoke\t80: Slowbro\n");
  printf("81: Magnemite\t82: Magneton\t83: Farfetch'd\t84: Doduo\t85: Dodrio\n");
  printf("86: Seel\t87: Dewgong\t88: Grimer\t89: Muk         90: Shellder\n");
  printf("91: Cloyster\t92: Gastly\t93: Haunter\t94: Gengar\t95: Onix\n");
  printf("96: Drowzee\t97: Hypno\t98: Krabby\t99: Kingler\t100: Voltorb\n");
  printf("101: Electrode\t102: Exeggcute\t103: Exeggutor\t104: Cubone\t"
	 "105: Marowak\n");
  printf("106: Hitmonlee\t107: Hitmonchan\t108: Lickitung\t109: Koffing\t"
	 "110: Weezing\n");
  printf("111: Rhyhorn\t112: Rhydon\t113: Chansey\t114: Tangela\t"
	 "115: Kangaskhan\n");
  printf("116: Horsea\t117: Seadra\t118: Goldeen\t119: Seaking\t120: Staryu\n");
  printf("121: Starmie\t122: Mr. Mime\t123: Scyther\t124: Jynx\t"
	 "125: Electabuzz\n");
  printf("126: Magmar\t127: Pinsir\t128: Tauros\t129: Magikarp\t130: Gyarados\n");
  printf("131: Lapras\t132: Ditto\t133: Eevee\t134: Vaporeon\t135: Jolteon\n");
  printf("136: Flareon\t137: Porygon\t138: Omanyte\t139: Omastar\t140: Kabuto\n");
  printf("141: Kabutops\t142: Aerodactyl\t143: Snorlax\t144: Articuno\t"
	 "145: Zapdos\n");
  printf("146: Moltres\t147: Dratini\t148: Dragonair\t149: Dragonite\t"
	 "150: Mewtwo\n");
  printf("151: Mew\n");
  //printf("824: Blipbug\t825: Dottler\t826: Orbeetle\n");
  if(!test){
    do{
      printf("ESCOLHA: ");
      scanf(" %d", &acao);
    }while(acao < 0);
  }
  else{
    if(team == 1)
      acao = test_pokemon1;
    else
      acao = test_pokemon2;
  }
  if(acao == 0)
    acao = (RAND() % 151) + 1;
  switch(acao){
  case 1:   return new_bulbasaur(team);
  case 2:   return new_ivysaur(team);
  case 3:   return new_venusaur(team);
  case 4:   return new_charmander(team);
  case 5:   return new_charmeleon(team);
  case 6:   return new_charizard(team);
  case 7:   return new_squirtle(team);
  case 8:   return new_wartortle(team);
  case 9:   return new_blastoise(team);
  case 10:  return new_caterpie(team);
  case 11:  return new_metapod(team);
  case 12:  return new_butterfree(team);
  case 13:  return new_weedle(team);
  case 14:  return new_kakuna(team);
  case 15:  return new_beedrill(team);
  case 16:  return new_pidgey(team);
  case 17:  return new_pidgeotto(team);
  case 18:  return new_pidgeot(team);
  case 19:  return new_rattata(team);
  case 20:  return new_raticate(team);
  case 21:  return new_spearow(team);
  case 22:  return new_fearow(team);
  case 23:  return new_ekans(team);
  case 24:  return new_arbok(team);
  case 25:  return new_pikachu(team);
  case 26:  return new_raichu(team);
  case 27:  return new_sandshrew(team);
  case 28:  return new_sandslash(team);
  case 29:  return new_nidoran_f(team);
  case 30:  return new_nidorina(team);
  case 31:  return new_nidoqueen(team);
  case 32:  return new_nidoran_m(team);
  case 33:  return new_nidorino(team);
  case 34:  return new_nidoking(team);
  case 35:  return new_clefairy(team);
  case 36:  return new_clefable(team);
  case 37:  return new_vulpix(team);
  case 38:  return new_ninetales(team);
  case 39:  return new_jigglypuff(team);
  case 40:  return new_wigglytuff(team);
  case 41:  return new_zubat(team);
  case 42:  return new_golbat(team);
  case 43:  return new_oddish(team);
  case 44:  return new_gloom(team);
  case 45:  return new_vileplume(team);
  case 46:  return new_paras(team);
  case 47:  return new_parasect(team);
  case 48:  return new_venonat(team);
  case 49:  return new_venomoth(team);
  case 50:  return new_digllet(team);
  case 51:  return new_dugtrio(team);
  case 52:  return new_meowth(team);
  case 53:  return new_persian(team);
  case 54:  return new_psyduck(team);
  case 55:  return new_golduck(team);
  case 56:  return new_mankey(team);
  case 57:  return new_primeape(team);
  case 58:  return new_growlithe(team);
  case 59:  return new_arcanine(team);
  case 60:  return new_poliwag(team);
  case 61:  return new_poliwhirl(team);
  case 62:  return new_poliwrath(team);
  case 63:  return new_abra(team);
  case 64:  return new_kadabra(team);
  case 65:  return new_alakazam(team);
  case 66:  return new_machop(team);
  case 67:  return new_machoke(team);
  case 68:  return new_machamp(team);
  case 69:  return new_bellsprout(team);
  case 70:  return new_weepinbell(team);
  case 71:  return new_victreebel(team);
  case 72: return new_tentacool(team);
  case 73: return new_tentacruel(team);
  case 74:  return new_geodude(team);
  case 75:  return new_graveler(team);
  case 76:  return new_golem(team);
  case 77:  return new_ponyta(team);
  case 78:  return new_rapidash(team);
  case 79:  return new_slowpoke(team);
  case 80:  return new_slowbro(team);
  case 81:  return new_magnemite(team);
  case 82:  return new_magneton(team);
  case 83:  return new_farfetchd(team);
  case 84:  return new_doduo(team);
  case 85:  return new_dodrio(team);
  case 86:  return new_seel(team);
  case 87:  return new_dewgong(team);
  case 88:  return new_grimer(team);
  case 89:  return new_muk(team);
  case 90:  return new_shellder(team);
  case 91:  return new_cloyster(team);
  case 92:  return new_gastly(team);
  case 93:  return new_haunter(team);
  case 94:  return new_gengar(team);
  case 95:  return new_onix(team);
  case 96:  return new_drowzee(team);
  case 97:  return new_hypno(team);
  case 98:  return new_krabby(team);
  case 99:  return new_kingler(team);
  case 100: return new_voltorb(team);
  case 101: return new_electrode(team);
  case 102: return new_exeggcute(team);
  case 103: return new_exeggutor(team);
  case 104: return new_cubone(team);
  case 105: return new_marowak(team);
  case 106: return new_hitmonlee(team);
  case 107: return new_hitmonchan(team);
  case 108: return new_lickitung(team);
  case 109: return new_koffing(team);
  case 110: return new_weezing(team);
  case 111: return new_rhyhorn(team);
  case 112: return new_rhydon(team);
  case 113: return new_chansey(team);
  case 114: return new_tangela(team);
  case 115: return new_kangaskhan(team);
  case 116: return new_horsea(team);
  case 117: return new_seadra(team);
  case 118: return new_goldeen(team);
  case 119: return new_seaking(team);
  case 120: return new_staryu(team);
  case 121: return new_starmie(team);
  case 122: return new_mrmime(team);
  case 123: return new_scyther(team);
  case 124: return new_jynx(team);
  case 125: return new_electabuzz(team);
  case 126: return new_magmar(team);
  case 127: return new_pinsir(team);
  case 128: return new_tauros(team);
  case 129: return new_magikarp(team);
  case 130: return new_gyarados(team);
  case 131: return new_lapras(team);
  case 132: return new_ditto(team);
  case 133: return new_eevee(team);
  case 134: return new_vaporeon(team);
  case 135: return new_jolteon(team);
  case 136: return new_flareon(team);
  case 137: return new_porygon(team);
  case 138: return new_omanyte(team);
  case 139: return new_omastar(team);
  case 140: return new_kabuto(team);
  case 141: return new_kabutops(team);
  case 142: return new_aerodactyl(team);
  case 143: return new_snorlax(team);
  case 144: return new_articuno(team);
  case 145: return new_zapdos(team);
  case 146: return new_moltres(team);
  case 147: return new_dratini(team);
  case 148: return new_dragonair(team);
  case 149: return new_dragonite(team);
  case 150: return new_mewtwo(team);
  case 151: return new_mew(team);
  case 824: return new_blipbug(team);
  case 825: return new_dottler(team);
  case 826: return new_orbeetle(team);
  default: return NULL;
  }
}

void player_choose_grapple_action(struct pokemon *p, struct pokemon *oponent){
  printf("  %d- Atacar\n", WITHERING_ATTACK);
  if(p -> initiative > 0)
    printf("  %d- %s\n", DECISIVE_ATTACK, p -> grapple_move);
  if(p -> control_turns > 2 && p -> grapple_move != glare)
    printf("  %d- Segurar Oponente\n", RESTRAIN);
  do{
    p -> action = NONE;
    printf("Escolha ação de %s: ", p -> name);
    scanf(" %d", &(p -> action));
    printf("%d %d\n", p -> action, p -> initiative);
  }while(p-> action != WITHERING_ATTACK &&
	 (p -> action != DECISIVE_ATTACK || p -> initiative <= 0) &&
	 (p -> action != RESTRAIN || p -> control_turns > 2));
}

void player_choose_action(struct pokemon *p, struct pokemon *oponent){
  int i;
  bool have_move;
  if(p -> control_turns > 0){
    player_choose_grapple_action(p, oponent);
    return;
  }
  check_valid_moves(p, oponent);
  p -> first_turn = false;
  if(p -> afraid >= 2){
    printf("%s está muito assustado.\n", p -> name);
    p -> afraid --;
    if(p -> afraid == 1)
      printf("Mas está começando a lidar melhor com o medo.\n");
  }
  if(p -> angry >= 2){
    printf("%s está furioso.\n", p -> name);
    p -> angry --;
    if(p -> angry == 1)
      printf("Mas está começando a lidar melhor com a raiva.\n");
  }
  if(p -> friendly >= 2){
    printf("%s está com medo de machucar %s.\n", p -> name, oponent -> name);
    p -> friendly --;
    if(p -> friendly == 1)
      printf("Mas está voltando a se focar na luta.\n");
  }
  if(p -> infatuated >= 2){
    printf("%s está apaixonado por %s.\n", p -> name, oponent -> name);
    p -> infatuated --;
    if(p -> infatuated == 1)
      printf("Mas está lembrando que precisa lutar.\n");
  }
  oponent -> first_turn = false;
  for(i = 0; i < NUMBER_OF_MOVES; i ++)
    if(p -> enabled_moves[i] == true)
      have_move = true;
  if(!have_move){
    p -> action = NONE;
    p -> restrained = false;
    return;
  }
  if(p -> action == AIM_ATT && p -> enabled_moves[AIM_ATT])
    return;
  if(p -> action == HIDE_ATT && p -> enabled_moves[HIDE_ATT])
    return;
  printf("AÇÕES (%s):\n", p -> name);
  if(p -> enabled_moves[WITHERING_ATTACK])
    printf("  %d- Atacar\n", WITHERING_ATTACK);
  if(p -> enabled_moves[DECISIVE_ATTACK]){
    i = (p -> initiative - 1) / 2;
    if(i > 25) i = 25;
    while(p -> description[i] == NULL)
      i --;
    printf("  %d- %s\n", DECISIVE_ATTACK, p -> description[i]);
  }
  if(p -> enabled_moves[CLASH_DECISIVE]){
    printf("  %d- Esperar ataque de %s e atacar\n", CLASH_WITHERING,
	   oponent -> name);
    printf("  %d- Esperar ataque de %s e usar %s\n", CLASH_DECISIVE,
	   oponent -> name, p -> description[i-1]);
  }
  if(p -> enabled_moves[FULL_DEFENSE])
    printf("  %d- %s\n", FULL_DEFENSE,  p -> full_defense_move);
  if(p -> enabled_moves[THREATEN])
    printf("  %d- %s\n", THREATEN,  p -> threaten_move);
  if(p -> enabled_moves[AIM])
    printf("  %d- Mirar\n", AIM);
  if(p -> enabled_moves[THROW_POISON])
    printf("  %d- %s\n", THROW_POISON,  p -> poison_move);
  if(p -> enabled_moves[MAKE_SLEEP])
    printf("  %d- %s\n", MAKE_SLEEP,  p -> sleep_move);
  if(p -> enabled_moves[SMOKE])
    printf("  %d- %s\n", SMOKE,  p -> smoke_move);
  if(p -> enabled_moves[CLEAN_EYES])
    printf("  %d- Limpar os Olhos\n", CLEAN_EYES);
  if(p -> enabled_moves[DISENGAGE])
    printf("  %d- Teleporte\n", DISENGAGE);
  if(p -> enabled_moves[ATT_DISENGAGE])
    printf("  %d- %s\n", ATT_DISENGAGE, p -> att_disengage_move);
  if(p -> enabled_moves[AIM_ATT])
    printf("  %d- %s\n", AIM_ATT, p -> aim_att_move);
  if(p -> enabled_moves[TAKE_COVER])
    printf("  %d- %s\n", TAKE_COVER, p -> take_cover_move);
  if(p -> enabled_moves[DIF_TERRAIN])
    printf("  %d- %s\n", DIF_TERRAIN, p -> dif_terrain_move);
  if(p -> enabled_moves[GRAPPLE])
    printf("  %d- %s\n", GRAPPLE, p -> grapple_move);
  if(p -> enabled_moves[SLAM])
    printf("  %d- %s\n", SLAM, p -> slam_move);
  if(p -> enabled_moves[GET_UP])
    printf("  %d- Levantar-se\n", GET_UP);
  if(p -> enabled_moves[AMBUSH])
    printf("  %d- %s\n", AMBUSH, p -> ambush_move);
  if(p -> enabled_moves[HIDE_ATT])
    printf("  %d- %s\n", HIDE_ATT, p -> hide_att_move);
  if(p -> enabled_moves[SCARE])
    printf("  %d- %s\n", SCARE, p -> scare_move);
  if(p -> enabled_moves[ANGER])
    printf("  %d- %s\n", ANGER, p -> anger_move);
  if(p -> enabled_moves[FRIENDSHIP])
    printf("  %d- %s\n", FRIENDSHIP, p -> friendship_move);
  if(p -> enabled_moves[FLIRT])
    printf("  %d- %s\n", FLIRT, p -> flirt_move);
  if(p -> enabled_moves[SEDUCE])
    printf("  %d- %s\n", SEDUCE,  p -> seduce_move);
  if(p -> enabled_moves[TRANSFORM])
    printf("  %d- Transformação\n", TRANSFORM);
  if(p -> enabled_moves[PARALYZE])
    printf("  %d- %s\n", PARALYZE, p -> paralysis_move);
  if(p -> enabled_moves[BURN])
    printf("  %d- %s\n", BURN, p -> burn_move);
  if(p -> enabled_moves[END_BURN])
    printf("  %d- Apagar o Fogo\n", END_BURN);
  do{
    p -> action = NONE;
    printf("Escolha ação de %s: ", p -> name);
    scanf(" %d", &(p -> action));
  }while(p-> action < 1 || p -> enabled_moves[p -> action] == false);
}

void choose_grapple_action(struct pokemon *p1, struct pokemon *p2){
  int i;
  float target_penalty;
  int rating[NUMBER_OF_MOVES];
  for(i = 0; i < NUMBER_OF_MOVES; i ++)
    rating[i] = 1000;
  target_penalty = (GET_DEFENSE(p2) + GET_BONUS_DEFENSE(p2, p1) -
		    0.5 * (GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1)));
  if(target_penalty < 1)
    target_penalty = 1;
  { // WITHERING_ATTACK
    int damage = 0;
    for(i = 0; i < 100; i ++){
      damage += simulate_withering_attack(p1, p2, 0, 0, NULL);
      if(damage >=  p2 -> initiative){
	rating[WITHERING_ATTACK] = i;
	break;
      }
    }
  }
  if(p1 -> initiative > 0){ // DECISIVE
    int defense;
    int my_initiative = p1 -> initiative;
    if(p1 -> enabled_moves[DECISIVE_ATTACK]){
      i = 0;
      do{
	int damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type,
					      0);
	defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
	if(damage >= 0){
	  if(p2 -> damage + damage > p2 -> vitality)
	    rating[DECISIVE_ATTACK] = i;
	  if(! p1 -> sleepy && p2 -> penalty[p2 -> damage] -
	     p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	    // Can I hold the counter-attack?
	    if(simulate_withering_attack(p2, p1,
					 p2 -> penalty[p2 -> damage + damage] -
					 p2 -> penalty[p2 -> damage], 0, NULL) <
	       3)
	      rating[DECISIVE_ATTACK] = i;
	  }
	  break;
	}
	else
	  my_initiative -= 2;
	i ++;
      } while(my_initiative >= p2 -> initiative);
    }
  }
  if(p1 -> control_turns > 2 && p1 -> grapple_move != glare){ // RESTRAIN
    if(p2 -> making_sleep)
      rating[RESTRAIN] = 1;
  }
  if(p1 -> slam_move && p1 -> initiative > 0){
    int attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
    int other_defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
    int damage, control;
    control = p1 -> control_turns;
    if(control > p1 -> strength + 1)
      control = p1 -> strength + 1;
    p1 -> initiative += (control - 1);
    damage = simulate_decisive_attack(p1, p2, 0, 0,
					p1 -> slam_type, 0);
    p1 -> initiative -= (control - 1);
    if(p2 -> damage + damage > p2 -> vitality)
      rating[SLAM] = 0;
    else if(p2 -> penalty[p2 -> damage] -
	    p2 -> penalty[p2 -> damage + damage] >= target_penalty){
      if(p2 -> shape != HEAD && p2 -> shape != FISH &&
	 p2 -> shape != FLYING_BUG &&
	 p2 -> shape != MULTIPLE_BODIES && p2 -> shape != OCTOPUS &&
	 p2 -> shape != HEAD_WITH_BASE && p2 -> shape != BIRD &&
	 p2 -> shape != HEAD_WITH_ARMS &&
	   (p1 -> size > p2 -> size || (p2 -> shape != INSECTOID &&
					p2 -> shape != QUADRUPED))){
	if(simulate_withering_attack(p2, p1,
				     p2 -> penalty[p2 -> damage + damage] -
				     p2 -> penalty[p2 -> damage] -3, 0,
				     NULL) < 3)
	  rating[SLAM] = 0;
      }
      else{
	if(simulate_withering_attack(p2, p1,
				     p2 -> penalty[p2 -> damage + damage] -
				     p2 -> penalty[p2 -> damage], 0,
				     NULL) < 3)
	  rating[SLAM] = 0;
      }
    }
  }
  { // CHOICE
    int choice[NUMBER_OF_MOVES];
    int number_of_choices = 0;
    int min_rating = 2000;
    for(i = 0; i < NUMBER_OF_MOVES; i ++)
      choice[i] = NONE;
    for(i = 1; i < NUMBER_OF_MOVES; i ++){
      if(rating[i] < min_rating){
	choice[0] = i;
	number_of_choices = 1;
	min_rating = rating[i];
      }
      else if(rating[i] == min_rating){
	choice[number_of_choices] = i;
	number_of_choices ++;
      }
    }
    if(number_of_choices == 1)
      p1 -> action = choice[0];
    else
      p1 -> action = choice[RAND() % number_of_choices];
  }
}

void choose_action(struct pokemon *p1, struct pokemon *p2){
  int i;
  float target_penalty;
  bool have_move;
  int rating[NUMBER_OF_MOVES];
  //printf("[BEGIN IA]\n");
  if(p1 -> control_turns > 0){
    choose_grapple_action(p1, p2);
    return;
  }
  check_valid_moves(p1, p2);
  p1 -> first_turn = false;
  p2 -> first_turn = false;
  if(p1 -> afraid >= 2){
    printf("%s está muito assustado.\n", p1 -> name);
    p1 -> afraid --;
    if(p1 -> afraid == 1)
      printf("Mas está começando a lidar melhor com o medo.\n");
  }
  if(p1 -> angry >= 2){
    printf("%s está furioso.\n", p1 -> name);
    p1 -> angry --;
    if(p1 -> angry == 1)
      printf("Mas está começando a lidar melhor com a raiva.\n");
  }
  if(p1 -> friendly >= 2){
    printf("%s está com medo de machucar %s.\n", p1 -> name, p2 -> name);
    p1 -> friendly --;
    if(p1 -> friendly == 1)
      printf("Mas está voltando a se concentrar na luta.\n");
  }
  if(p1 -> infatuated >= 2){
    printf("%s está apaixonado por %s.\n", p1 -> name, p2 -> name);
    p1 -> friendly --;
    if(p1 -> friendly == 1)
      printf("Mas está lembrando que deve lutar.\n");
  }
  for(i = 0; i < NUMBER_OF_MOVES; i ++)
    if(p1 -> enabled_moves[i] == true)
      have_move = true;
  if(!have_move){
    p1 -> action = NONE;
    p1 -> restrained = false;
    return;
  }
  if(p1 -> action == AIM_ATT && p1 -> enabled_moves[AIM_ATT])
    return;
  if(p1 -> action == HIDE_ATT && p1 -> enabled_moves[HIDE_ATT])
    return;
  for(i = 0; i < NUMBER_OF_MOVES; i ++)
    rating[i] = 1000;
  rating[WITHERING_ATTACK] = 999;
  target_penalty = (GET_DEFENSE(p2) + GET_BONUS_DEFENSE(p2, p1) -
		    0.5 * (GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1)));
  if(target_penalty < 1)
    target_penalty = 1;
  { // Begin Decisive Attack Rating
    int defense, j;
    int my_initiative = p1 -> initiative;
    if(p1 -> enabled_moves[DECISIVE_ATTACK]){
      for(j = 25; j > 0; j --)
	if(p1 -> type[j] != NONE)
	  break;
      if(compute_weakness(p1 -> type[j], p2) > 2){ // Último  ataque  inútil
	for(; j >=0; j --)
	  if(compute_weakness(p1 -> type[j], p2) < 2)
	    break;
	if((p1 -> initiative - 1) / 2 >= j)
	  rating[DECISIVE_ATTACK] = -1;
      }
      else{
	i = 0;
	do{
	  int damage = simulate_decisive_attack(p1, p2, 0, 0, NONE, DEFAULT);
	  defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
	  char *move;
	  int k = 0;
	  if(p1 -> initiative > 0){
	    k = (p1 -> initiative - 1) / 2;
	    if(k > 25) k = 25;
	    while(p1 -> description[k] == NULL)
	      k --;
	  }
	  move = p1 -> description[k];
	  if(damage >= 0){
	    if(p2 -> damage + damage > p2 -> vitality)
	      rating[DECISIVE_ATTACK] = i;
	    if(! (p1 -> sleepy) && p2 -> penalty[p2 -> damage] -
	       p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	      // Can I hold the counter-attack or my attack get consequence?
	      if(simulate_withering_attack(p2, p1,
					   p2 -> penalty[p2 -> damage + damage] -
					   p2 -> penalty[p2 -> damage], 0, NULL) <
		 3 || attack_consequences(p1, p2, move) != NONE)
		rating[DECISIVE_ATTACK] = i;
	    }  
	    break;
	  }
	  else
	    my_initiative -= 2;
	  i ++;
	} while(my_initiative >= p2 -> initiative);
      }
    }
  } // End Decisive Attack Rating
  //printf("IA Decisive: %d\n", rating[DECISIVE_ATTACK]);
  { // Begin Withering Attack Rating
    int defense;
    int enemy_initiative = p2 -> initiative;
    int attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
    for(i = 0; i < 100; i ++){
      int defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
      if(roll(attack) >= defense){
	int damage = roll(p1 -> initiative);
	if(damage < 0) damage = 0;
	enemy_initiative -= damage;
	if(enemy_initiative <= 0){
	  rating[WITHERING_ATTACK] = i;
	  break;
	}
      }
    }
  } // End Withering Attack Rating
  //printf("IA Withering: %d\n", rating[WITHERING_ATTACK]);
  if(p1 -> enabled_moves[CLASH_WITHERING] && GET_BONUS_DEFENSE_INT(p2, p1) &&
     p1 -> aiming == 0){
    bool dummy;
    int damage;
    p1 -> initiative -= 2;
    damage = simulate_clash_attack(p1, p2, WITHERING_ATTACK, WITHERING_ATTACK,
				   NONE, NONE,  0, 0, &dummy);
    p1 -> initiative += 2;
    if(damage > 2){
      if(p2 -> initiative > 0)
	rating[CLASH_WITHERING] = (p2 -> initiative / damage);
      else
	rating[CLASH_WITHERING] = 1;
    }
  } // End Clash Withering Rating
  //printf("IA Clash Withering: %d\n", rating[CLASH_WITHERING]);
  if(p1 -> enabled_moves[CLASH_DECISIVE] && p1 -> initiative > 2 &&
     GET_BONUS_DEFENSE_INT(p2, p1) && p1 -> aiming == 0){
    bool dummy;
    int damage;
    p1 -> initiative -= 2;
    damage = simulate_clash_attack(p1, p2, DECISIVE_ATTACK, WITHERING_ATTACK,
				   NONE, NONE,  0, 0, &dummy);
    p1 -> initiative += 2;
    if(damage > 0){
      if(p2 -> damage + damage > p2 -> vitality)
	rating[CLASH_DECISIVE] = 0;
      else if(! p1 -> sleepy && p2 -> penalty[p2 -> damage] -
	      p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	rating[CLASH_DECISIVE] = 0;
      }
    }
  } // End Clash Decisive Rating
  //printf("IA Clash Decisive: %d\n", rating[CLASH_DECISIVE]);
  if(p1 -> enabled_moves[THREATEN]){ // Begin Threaten rating
    int success, resolve;
    if(p1 -> hideous)
      success = roll(GET_THREATEN_DICE(p1) + GET_SOCIAL_BONUS(p1, p2));
    else
      success = roll(GET_THREATEN_DICE(p1));
    resolve = GET_RESOLVE(p2) + 4;
    if(p2 -> afraid)
      resolve -= 2;
    if(success >= resolve)
      rating[THREATEN] = 0;
  } // End Threaten rating
  if(p1 -> enabled_moves[SEDUCE]){ // Begin Threaten rating
    int success, resolve;
    if(!(p1 -> hideous))
      success = roll(GET_THREATEN_DICE(p1) + GET_SOCIAL_BONUS(p1, p2));
    else
      success = roll(GET_THREATEN_DICE(p1));
    resolve = GET_RESOLVE(p2) + 4;
    if(p2 -> infatuated)
      resolve -= 2;
    if(success >= resolve)
      rating[SEDUCE] = 0;
  } // End Threaten rating
  if(p1 -> enabled_moves[BURN] && p1 -> initiative > 7){ // Begin Burn
    if(simulate_decisive_attack(p1, p2, 0, 0, POISON, 1) >=3)
      rating[BURN] = 0;
  }
  if(p1 -> enabled_moves[END_BURN])
    rating[END_BURN] = -1;
  if(p1 -> enabled_moves[THROW_POISON] && p1 -> initiative > 7 &&
     p2 -> paralyzed < 2){ // Begin poison
    if(simulate_decisive_attack(p1, p2, 0, 0, POISON, 1) >=3)
      rating[THROW_POISON] = 0;
  } // End poison
  if(p1 -> enabled_moves[PARALYZE] && p1 -> initiative > 7 &&
     p2 -> poisoned < 2){ // Begin paralyze
    int type;
    if(p1 -> paralysis_move == thunder_wave)
      type = ELECTRIC;
    else if(p1 -> paralysis_move == stun_spore)
      type = GRASS;
    if(simulate_decisive_attack(p1, p2, 0, 0, type, 1) >=3)
      rating[PARALYZE] = 0;
  } // End paralyze
  if(p1 -> enabled_moves[MAKE_SLEEP] && GET_MAGIC_DICE(p1) > 0){ // Begin Sleep
    if(p1 -> making_sleep == false){
      int dice_pool = GET_MAGIC_DICE(p1);
      int motes = 0;
      i = -1;
      do{
	motes += roll(dice_pool);
	i ++;
      }while(motes < 7);
      rating[MAKE_SLEEP] = i;
    }
    else rating[MAKE_SLEEP] = -1;
  } // End Sleep
  if(p1 -> enabled_moves[SMOKE]){ //Begin Smoke
    if(p1 -> initiative > p2 -> initiative + 2){
      if(roll(GET_ATTACK_DICE(p1)+GET_BONUS_ATTACK(p1)) >
	 roll(GET_ATTACK_DICE(p2)+GET_BONUS_ATTACK(p2)))
	rating[SMOKE] = 0;
    }
  } // End Smoke
  if(p1 -> enabled_moves[CLEAN_EYES]){ // Clean Eyes Begin
    if(p1 -> blinded)
      rating[CLEAN_EYES] = -1;
  } // Clean Eyes
  if(p1 -> enabled_moves[AIM]){
    if(p1 -> enabled_moves[DECISIVE_ATTACK]){
      // Aimed decisive deals enough damage?
      int my_initiative = p1 -> initiative;
      int damage = simulate_decisive_attack(p1, p2, 3, 0, NONE, DEFAULT);
      int attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
      int defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
      if(damage >= 0){
	if(p2 -> damage + damage > p2 -> vitality)
	  rating[AIM] = 2;
	else if(p1 -> sleepy == 0 &&
		p2 -> penalty[p2 -> damage] -
		p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	  if(simulate_withering_attack(p2, p1,
				       p2 -> penalty[p2 -> damage + damage] -
				       p2 -> penalty[p2 -> damage], 0, NULL) < 3)
	    rating[AIM] = 2;
	}
      }
    }
    // Better aim or better attack 2x?
    {
      int my_aim_attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1) + 3;
      int my_attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
      int other_defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
      int damage, aim_damage = 0, normal_damage = 0, success;
      success = roll(my_aim_attack);
      if(success >= other_defense){
	if(p1 -> strength - p2 -> stamina >= p1 -> inteligence - p2 -> wits)
	  aim_damage = p1 -> strength + success - other_defense - p2 -> stamina;
	else
	  aim_damage = p1 -> inteligence + success - other_defense - p2 -> wits;
	if(aim_damage < 1)
	  aim_damage = 1;
	aim_damage = roll(aim_damage);
      }
      for(i = 0; i < 2; i ++){
	success = roll(my_attack);
	if(success >= other_defense){
	  if(p1 -> strength - p2 -> stamina >= p1 -> inteligence - p2 -> wits)
	    damage = p1 -> strength + success - other_defense - p2 -> stamina;
	  else
	    damage = p1 -> inteligence + success - other_defense - p2 -> wits;
	  if(damage < 1)
	    damage = 1;
	  normal_damage += roll(damage);
	}
      }
      if(aim_damage > normal_damage){
	if(rating[WITHERING_ATTACK] - 1 < rating[AIM])
	  rating[AIM] = rating[WITHERING_ATTACK] - 1;
      }
    }
  }
  //printf("IA Aim: %d\n", rating[AIM]);
  if(p1 -> enabled_moves[FULL_DEFENSE] && p1 -> initiative > 2){
    // What is better to the initiative: attack and being attacked or
    // lose 2 initiative and being attacked with higher defense?
    int my_attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
    int other_attack = GET_ATTACK_DICE(p2) + GET_BONUS_ATTACK(p2);
    int my_defense = GET_DEFENSE_INT(p1) + GET_BONUS_DEFENSE_INT(p1, p2);
    int other_defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
    int initiative_normal = 0, initiative_defense = -2, damage;
    damage = roll(my_attack);
    if(damage >= other_defense){
      if(p1 -> strength - p2 -> stamina >= p1 -> inteligence - p2 -> wits)
	damage = p1 -> strength + damage - other_defense - p2 -> stamina;
      else
	damage = p1 -> inteligence + damage - other_defense - p2 -> wits;
      if(damage < 1)
	damage = 1;
      initiative_normal += roll(damage);
    }
    damage = roll(other_attack);
    if(damage >= my_defense){
      if(p2 -> strength - p1 -> stamina >= p2 -> inteligence - p1 -> wits)
	damage = p2 -> strength + damage - my_defense - p1 -> stamina;
      else
	damage = p2 -> inteligence + damage - my_defense - p1 -> wits;
      if(damage< 1)
	damage = 1;
      initiative_normal -= roll(damage);
    }
    damage = roll(other_attack);
    if(damage >= my_defense + 2){
      if(p2 -> strength - p1 -> stamina >= p2 -> inteligence - p1 -> wits)
	damage = p2 -> strength + damage - my_defense - p1 -> stamina - 2;
      else
	damage = p2 -> inteligence + damage - my_defense - p1 -> wits - 2;
      if(damage< 1)
	damage = 1;
      initiative_defense -= roll(damage);
    }
    if(initiative_defense > initiative_normal)
      rating[FULL_DEFENSE] = 0;
  }
  if(p1 -> enabled_moves[DISENGAGE] && (p1 -> initiative > 2 ||
					p1 -> initiative < 0)){ // Teleport
    int i1, i2;
    i1 = (p1 -> initiative - 1) / 2;
    if(i1 > 25) i1 = 25;
    while(p1 -> description[i1] == NULL)
      i1 --;
    i2 = (p2 -> initiative - 1) / 2;
    if(i2 > 25) i2 = 25;
    while(p2 -> description[i2] == NULL)
      i2 --;
    if(p2 -> inteligence <= p2 -> strength && p1 -> ranged[i1] &&
       !(p2 -> ranged[i2])){
      if(roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1)) >
	 roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2)))
	rating[DISENGAGE] = 1;
    }
  }
  if(p1 -> enabled_moves[ATT_DISENGAGE] && p1 -> initiative > 2){
    int d1 = simulate_decisive_attack(p1, p2, 0, 0, NONE, DEFAULT);
    int d2 = simulate_decisive_attack(p1, p2, -3, 0, p1 -> att_disengage_type,
				      p1 -> att_disengage_ranged);
    if(d2 > d1)
      rating[ATT_DISENGAGE] = rating[DECISIVE_ATTACK] - 1;
  }
  if(p1 -> enabled_moves[AIM_ATT] && p1 -> initiative > 2){
    int my_initiative = p1 -> initiative;
    my_initiative -= simulate_withering_attack(p1, p2, 0, 0, NULL);
    if(my_initiative < p2 -> initiative && p1 -> initiative > p2 -> initiative)
      my_initiative -= simulate_withering_attack(p1, p2, 0, 0, NULL);
    if(my_initiative > 2){
      int damage, save_initiative = p1 -> initiative;
      p1 -> initiative = my_initiative;
      damage = simulate_decisive_attack(p1, p2, 3, 0, p1 -> aim_att_type,
					p1 -> aim_att_ranged);
      p1 -> initiative = save_initiative;
      if(p2 -> damage + damage > p2 -> vitality)
	rating[AIM_ATT] = 2;
      else if(!(p2 -> sleepy) && p2 -> penalty[p2 -> damage] -
	      p2 -> penalty[p2 -> damage + damage] >=
	      target_penalty){
	if(simulate_withering_attack(p2, p1,
				     p2 -> penalty[p2 -> damage + damage] -
				     p2 -> penalty[p2 -> damage], 0, NULL) < 3)
	  rating[AIM_ATT] = 2;
      }
	
    }
  }
  if(p1 -> enabled_moves[TAKE_COVER] && p1 -> initiative > p2 -> initiative){
    int j = (p1 -> initiative - 1) / 2;
    if(j > 25) j = 25;
    while(p1 -> description[j] == NULL)
      j --;
    if(GET_ATTACK_TYPE(p1, p2) != PHYSICAL && p1 -> ranged[j]){
      int result = roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1));
      if(result > 3)
	result = 3;
      if(result > 0)
	rating[TAKE_COVER] = 4 - result;
    }
  }
  if(p1 -> enabled_moves[DIF_TERRAIN]){
    bool lose = (roll(GET_ATTACK_DICE(p1) + GET_BONUS_MOVE(p1)) <
		 roll(GET_ATTACK_DICE(p2) + GET_BONUS_MOVE(p2)));
    if((p2 -> can_teleport && lose) || (p2 -> smoke_move != NULL && lose) ||
       p2 -> take_cover_move || (p1 -> can_teleport && lose) ||
       (p1 -> smoke_move != NULL && lose)){
      rating[DIF_TERRAIN] = rating[DISENGAGE] - 1;
      if(rating[SMOKE] - 1 < rating[DIF_TERRAIN])
	rating[DIF_TERRAIN] = rating[SMOKE] - 1;
      if(rating[DIF_TERRAIN] > 5)
	rating[DIF_TERRAIN] = 5;
    }
  }
  if(p1 -> enabled_moves[GRAPPLE] && p1 -> initiative > 5){
    int attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
    int other_defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
    if(simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type, 0) >= 2){
      int grapple_initiative = 0, normal_initiative = 0, damage = 0;
      grapple_initiative += simulate_withering_attack(p1, p2, 0, -other_defense,
						      NULL) - 3;
      grapple_initiative -= simulate_withering_attack(p2, p1, 0, -2, NULL);
      normal_initiative += simulate_withering_attack(p1, p2, 0, 0, NULL);
      normal_initiative -= simulate_withering_attack(p2, p1, 0, 0, NULL);
      if(grapple_initiative > normal_initiative)
	rating[GRAPPLE] = rating[WITHERING_ATTACK] - 1;
      p1 -> initiative -= 3;
      damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type, 0);
      p1 -> initiative += 3;
      if(p2 -> damage + damage > p2 -> vitality && rating[DECISIVE_ATTACK] != 0)
	rating[GRAPPLE] = 0;
      else if(!(p1 -> sleepy) && p2 -> penalty[p2 -> damage] -
	      p2 -> penalty[p2 -> damage + damage] >= target_penalty &&
	      rating[DECISIVE_ATTACK] != 0)
	rating[GRAPPLE] = 0;
      else if(p2 -> making_sleep || p1 -> grapple_move == glare){
	int control;
	if(p1 -> grapple_category == PHYSICAL)
	  control = roll(GET_STRENGTH_DICE(p1)) - roll(GET_STRENGTH_DICE(p2));
	else
	  control = roll(GET_INTELIGENCE_DICE(p1)) -
	    roll(GET_INTELIGENCE_DICE(p2));
	if(control > 2)
	  rating[GRAPPLE] = 0;
      }
    }
  }
  if((p1 -> enabled_moves[SLAM] ||
      (p1 -> enabled_moves[GRAPPLE] && p1 -> slam_move != NULL))
     && p1 -> initiative > 5){
    int attack = GET_ATTACK_DICE(p1) + GET_BONUS_ATTACK(p1);
    int other_defense = GET_DEFENSE_INT(p2) + GET_BONUS_DEFENSE_INT(p2, p1);
    if(simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type, 0) >= 2){
      // Conseguiu pegar
      int damage;
      int control = roll(GET_STRENGTH_DICE(p1)) - roll(GET_STRENGTH_DICE(p2));
      if(control > p1 -> strength)
	control = p1 -> strength;
      p1 -> initiative += (control - 3);
      damage = simulate_decisive_attack(p1, p2, 0, -other_defense,
					p1 -> slam_type, 0);
      p1 -> initiative -= (control - 3);
      if(p2 -> damage + damage > p2 -> vitality){
	if(p1 -> enabled_moves[SLAM])
	  rating[SLAM] = 0;
	else
	  rating[GRAPPLE] = 0;
      }
      else if(p2 -> penalty[p2 -> damage] -
	      p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	if(p2 -> shape != HEAD && p2 -> shape != FISH &&
	   p2 -> shape != FLYING_BUG &&
	   p2 -> shape != MULTIPLE_BODIES && p2 -> shape != OCTOPUS &&
	   p2 -> shape != HEAD_WITH_BASE && p2 -> shape != BIRD &&
	   p2 -> shape != HEAD_WITH_ARMS &&
	   (p1 -> size > p2 -> size || (p2 -> shape != INSECTOID &&
					p2 -> shape != QUADRUPED))){
	  if(simulate_withering_attack(p2, p1,
				       p2 -> penalty[p2 -> damage + damage] -
				       p2 -> penalty[p2 -> damage] -3, 0,
				       NULL) < 3){
	    if(p1 -> enabled_moves[SLAM])
	      rating[SLAM] = 0;
	    else
	      rating[GRAPPLE] = 0;
	  }
	}
	else{
	  if(simulate_withering_attack(p2, p1,
				       p2 -> penalty[p2 -> damage + damage] -
				       p2 -> penalty[p2 -> damage], 0,
				       NULL) < 3){
	    if(p1 -> enabled_moves[SLAM])
	      rating[SLAM] = 0;
	    else
	      rating[GRAPPLE] = 0;
	  }
	}
      }
    }
  }
  if(p1 -> enabled_moves[GET_UP])
    rating[GET_UP] = -1;
  if(p1 -> enabled_moves[AMBUSH] && p1 -> ambush_attack == WITHERING_ATTACK)
    rating[AMBUSH] = -1;
  if(p1 -> enabled_moves[HIDE_ATT]){
    if(roll(GET_ATTACK_DICE(p1) + GET_BONUS_STEALTH(p1) - 3) >
       roll(GET_PERCEPTION_DICE(p2) + GET_BONUS_PERCEPTION(p2))){
      int damage = simulate_decisive_attack(p1, p2, 0, -2, p1 -> hide_att_type,
					    0);
      if(p2 -> damage + damage > p2 -> vitality)
	rating[HIDE_ATT] = 1;
      else if(p2 -> penalty[p2 -> damage] -
	      p2 -> penalty[p2 -> damage + damage] >= target_penalty){
	if(simulate_withering_attack(p2, p1, 0,
				     - (p2 -> penalty[p2 -> damage + damage] -
					p2 -> penalty[p2 -> damage]),
				     NULL) < 3)
	  rating[HIDE_ATT] = 1;
      }
    }
  }
  if(p1 -> enabled_moves[SCARE] ||
     p1 -> enabled_moves[FRIENDSHIP] || p1 -> enabled_moves[FLIRT]){
    int resolve, bonus = 0, result;
    resolve = GET_RESOLVE_INT(p2);
    result = roll(GET_THREATEN_DICE(p1) + bonus);
    if(p1 -> willpower > 0 && p2 -> willpower == 0){
      result ++;
    }
    if(result - resolve > 0){
      if(p1 -> enabled_moves[SCARE] && p2 -> friendly < 2 &&
	 p2 -> infatuated < 2 && p2 -> angry < 2)
      rating[SCARE] = 0;
      if(p1 -> enabled_moves[FRIENDSHIP] && p2 -> angry < 2 &&
	 p2 -> infatuated < 2 && p2 -> afraid < 2)
      rating[FRIENDSHIP] = 0;
      if(p1 -> enabled_moves[FLIRT] && (!(p1 -> enabled_moves[FRIENDSHIP]) ||
					p2 -> friendly) &&
	 (p1 -> flirt_move == p1 -> flirt2 || RAND() % 2) &&
	 p2 -> angry < 2 &&
	 p2 -> infatuated < 2 && p2 -> afraid < 2)
	rating[FLIRT] = 0;
    }
    else if(result == resolve && p1 -> threaten_move != NULL)
      rating[SCARE] = 1;
  }
  if(p1 -> enabled_moves[ANGER] && p2 -> making_sleep){
    int resolve, result;
    resolve = GET_RESOLVE_INT(p2);
    if(p2 -> afraid)
      resolve += 3;
    result = roll(GET_MANIPULATION_DICE(p1));
    if(p1 -> willpower > 0 && p2 -> willpower == 0){
      result ++;
    }
    if(result - resolve > 0)
      rating[ANGER] = 0;
  }
  if(p1 -> enabled_moves[TRANSFORM]){
    if(!strcmp(p1 -> name, "Ditto") && strcmp(p2 -> name, "Ditto"))
      rating[TRANSFORM] = 0;
    int sum = (p2 -> strength > p1 -> strength) +
      (p2 -> dexterity > p1 -> dexterity) +
      (p2 -> stamina > p1 -> stamina) + (p2 -> charisma > p1 -> charisma) +
      (p2 -> manipulation > p1 -> manipulation) + (p2 -> wits > p1 -> wits) +
      (p2 -> appearance > p1 -> appearance) +
      (p2 -> perception > p1 -> perception) +
      (p2 -> inteligence > p1 -> inteligence);
    if(sum > 0 || p2 -> essence > p1 -> essence)
      rating[TRANSFORM] = 0;
  }
  //printf("IA Transform: %d\n", rating[TRANSFORM]);
  // Correções e ajustes
  if(p1 -> initiative <= 2){
    rating[DECISIVE_ATTACK] ++;
    rating[CLASH_DECISIVE] ++;
  }
  if(GET_ATTACK_DICE(p2) + GET_BONUS_ATTACK(p2) <= 0){
    rating[DECISIVE_ATTACK] --;
  }
  for(i = 0; i < NUMBER_OF_MOVES; i ++)
    if(!(p1 -> enabled_moves[i]))
      rating[i] = 5000;
  //printf("[IA ADJUSTED]\n");
  {
    int choice[NUMBER_OF_MOVES];
    int number_of_choices = 0;
    int min_rating = 2000;
    for(i = 0; i < NUMBER_OF_MOVES; i ++)
      choice[i] = NONE;
    for(i = 1; i < NUMBER_OF_MOVES; i ++){
      if(rating[i] < min_rating){
	choice[0] = i;
	number_of_choices = 1;
	min_rating = rating[i];
      }
      else if(rating[i] == min_rating){
	choice[number_of_choices] = i;
	number_of_choices ++;
      }
    }
    if(number_of_choices == 0)
      p1 -> action = NONE;
    else if(number_of_choices == 1)
      p1 -> action = choice[0];
    else
      p1 -> action = choice[RAND() % number_of_choices];
  }
}

void grapple(struct pokemon *p1, struct pokemon *p2){
  int damage, control;
  printf("%s está tentando prender %s com '%s'.\n", p1 -> name, p2 -> name,
	 p1 -> grapple_move);
  damage = simulate_decisive_attack(p1, p2, 0, 0, p1 -> grapple_type, 0);
  if(damage < 0){
    printf("Mas %s se desviou e %s perdeu 5 de Iniciativa.\n",
	   p2 -> name, p1 -> name);
    add_initiative(p1, p2, -5, true);
    return;
  }
  add_initiative(p1, p2, -3, true);
  if(damage < 2){
    printf("Mas não conseguiu segurar %s.\n", p2 -> name);
    return;
  }
  printf("Conseguiu!\n");
  if(p1 -> grapple_category == PHYSICAL)
    control = roll(GET_STRENGTH_DICE(p1)) - roll(GET_STRENGTH_DICE(p2));
  else
    control = roll(GET_INTELIGENCE_DICE(p1)) - roll(GET_INTELIGENCE_DICE(p2));
  if(control < 0)
    control = 1;
  else
    control ++;
  p1 -> control_turns = control;
  p2 -> grappled = true;
  if(p1 -> ia)
    choose_grapple_action(p1, p2);
  else
    player_choose_grapple_action(p1, p2);
  switch(p1 -> action){
  case WITHERING_ATTACK:
    withering_attack(p1, p2);
    break;
  case DECISIVE_ATTACK:
    decisive_attack(p1, p2);
    break;
  case RESTRAIN:
    printf("%s está impedindo ações de %s.\n", p1 -> name, p2 -> name);
    p1 -> control_turns -= 2;
    p2 -> restrained = true;
    break;
  default:
    break;
  }
}

void adapt_pokemon(struct pokemon *p1, struct pokemon *p2){
  if(!strcmp(p1 -> name, "Onix")){
    if(compute_weakness(GROUND, p2) < compute_weakness(NORMAL, p2)){
      p1 -> grapple_move = sand_tomb;
      p1 -> grapple_type = GROUND;
      p1 ->  grapple_category = PHYSICAL;
    }
  }
  if(!strcmp(p1 -> name, "Dratini") || !strcmp(p1 -> name, "Dragonair") ||
     !strcmp(p1 -> name, "Dragonite")){
    if(compute_weakness(DRAGON, p2) < compute_weakness(NORMAL, p2)){
      p1 -> slam_move = dragon_tail;
      p1 -> slam_type = DRAGON;
    }
  }
  if(!strcmp(p1 -> name, "Ekans") || !strcmp(p1 -> name, "Arbok")){
    if(p2 -> type1 == ELECTRIC || p2 -> type2 == ELECTRIC || p2 -> other_senses){
      if(p2 -> type1 != GHOST && p2 -> type2 != GHOST){
	p1 -> grapple_move = wrap;
	p1 -> grapple_type = NORMAL;
	p1 ->  grapple_category = PHYSICAL;
      }
      else{
	p1 -> grapple_move = NULL;
      }
    }
  }

  if(p1 -> threaten_move != NULL || p1 -> scare_move != NULL)
    p1 -> hideous = true;
  p1 -> poison_remaining = (p1 -> stamina + 1) / 2;
}

int main(int argc, char **argv){
  test = false;
  int number_of_players;
  int acao, i, rodada = 0, turno = 0;
  struct pokemon *p1, *p2, *order[2];
  {
    uint64_t buffer[4];
#if defined(__linux__)
    getrandom(buffer, 4 * 8, 0);
#elif defined(__EMSCRIPTEN__)
  for(i = 0; i < 4; i ++){
    buffer[i] = EM_ASM_INT({
	var array = new Uint32Array(1);
	window.crypto.getRandomValues(array);
	return array[0];
      });
    buffer[i] = buffer[i] << 32;
    buffer[i] += EM_ASM_INT({
	var array = new Uint32Array(1);
	window.crypto.getRandomValues(array);
	return array[0];
      });
  }
#elif defined(_WIN32)
  NTSTATUS ret;
  int count = 0;
  do{
    ret = BCryptGenRandom(NULL, (unsigned char *) &buffer, 8 * 4,
                          BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    count ++;
  } while(ret != 0 && count < 16);
  if(ret != 0){
    fprintf(stderr, "ERROR: I could not initialize the RNG.\n");
    exit(1);
  }
#else // BSD?
    arc4random_buf(buffer, 4 * 8);
#endif
    RNG = _Wcreate_rng(malloc, 4, buffer);
  }
  if(argc == 2  && atoi(argv[1]) == 0)
    number_of_players = 0;
  else if(argc == 2 && atoi(argv[1]) == 2)
    number_of_players = 2;
  else if(argc == 3)
    number_of_players = 0;
  else
    number_of_players = 1;
  if(argc >= 3){
    test = true;
    test_pokemon1 = atoi(argv[1]);
    test_pokemon2 = atoi(argv[2]);
  }
  printf("ESCOLHA POKÉMON DO JOGADOR 1:\n");
  p1 = escolhe_pokemon(1);
  printf("ESCOLHA POKÉMON DO JOGADOR 2:\n");
  p2 = escolhe_pokemon(2);
  adapt_pokemon(p1, p2);
  adapt_pokemon(p2, p1);
  if(!strcmp(p1 -> name, p2 -> name)){
    strcat(p1 -> name, " 1");
    strcat(p2 -> name, " 2");
  }
  if(number_of_players > 1)
    p2 -> ia = false;
  else if(number_of_players > 0)
    p1 -> ia = false;
  do {
    rodada ++;
    turno = 0;
    ////
    if(p1 -> initiative == p2 -> initiative){
      if(RAND() % 2){
	order[0] = p1;
	order[1] = p2;
      }
      else{
	order[0] = p2;
	order[1] = p1;
      }
    }
    else if(p1 -> initiative > p2 -> initiative){
      order[0] = p1;
      order[1] = p2;
    }
    else{
      order[0] = p2;
      order[1] = p1;
    }
    //////
    for(i = 0; i < 2; i ++){
      turno ++;
      if(order[i] -> damage > order[i] -> vitality || order[i] -> forfeit)
	break;
      printf("\n\n\n\n\n\n");
      printf("RODADA: %d     TURNO: %d\n", rodada, turno);
      if(i == 0 && p1 -> initiative == p2 -> initiative){
	int j;
	p1 -> onslaugh_penalty = 0;
	p2 -> onslaugh_penalty = 0;
	p1 -> full_defense = false;
	p2 -> full_defense = false;
	if(p1 -> distance > 0) p1 -> distance --;
	if(p2 -> distance > 0) p2 -> distance --;
	if(p1 -> control_turns){
	  p1 -> control_turns --;
	  if(p1 -> control_turns == 0){
	    p2 -> grappled = false;
	    printf("%s escapou.\n", p2 -> name);
	  }
	}
	if(p2 -> control_turns){
	  p2 -> control_turns --;
	  if(p2 -> control_turns == 0){
	    p1 -> grappled = false;
	    printf("%s escapou.\n", p1 -> name);
	  }
	}
	print_pokemon(p1, p2);
	print_pokemon(p2, p1);
	for(j = 0; j < 2; j ++){
	  //printf("[CHOOSE %s]\n", order[j] -> name);
	  if(order[j] == p1){
	    if(number_of_players > 0)
	      player_choose_action(order[j], order[!j]);
	    else
	      choose_action(order[j], order[!j]);
	  }
	  else{
	    if(number_of_players > 1)
	      player_choose_action(order[j], order[!j]);
	    else
	      choose_action(order[j], order[!j]);
	  }
	  //printf("[CHOSEN %s]\n", order[j] -> name);
	}
	if((IS_ATTACK(p1 -> action) ||
	    (p1 -> action == AIM_ATT && p1 -> aiming > 0)) &&
	   (IS_ATTACK(p2 -> action) ||
	    (p2 -> action == AIM_ATT && p2 -> aiming > 0))){
	  clash_attack(p1, p2);
	  i ++;
	  GETCHAR();
	  continue;
	}
      }
      else if(order[i] -> action == NONE){
	order[i] -> full_defense = false;
	order[i] -> onslaugh_penalty = 0;
	if(order[i] -> distance > 0) order[i] -> distance --;
	print_pokemon(p1, p2);
	print_pokemon(p2, p1);
	if(order[i] == p1){
	  if(number_of_players > 0)
	    player_choose_action(p1, p2);
	  else
	    choose_action(p1, p2);
	}
	else{
	  if(number_of_players > 1)
	    player_choose_action(p2, p1);
	  else
	    choose_action(p2, p1);

	}
      }
      if(order[i] -> action == CLASH_WITHERING ||
	 order[i] -> action == CLASH_DECISIVE){
	printf("%s está esperando o ataque de %s para atacar.\n",
	       order[i] -> name, order[!i] -> name);
	printf("Perdeu 2 de Iniciativa por esperar.\n");
	add_initiative(order[i], order[!i], -2, true);
      }
      else if(order[!i] -> action == CLASH_WITHERING ||
	      order[!i] -> action == CLASH_DECISIVE){
	if(IS_ATTACK(order[i] -> action)){
	  clash_attack(p1, p2);
	}
      }
      else if(order[i] -> action == WITHERING_ATTACK)
	withering_attack(order[i], order[!i]);
      else if(order[i] -> action == DECISIVE_ATTACK)
	decisive_attack(order[i], order[!i]);
      if(order[i] -> action == AIM){
	order[i] -> aiming = 2;
	printf("%s está mirando seu próximo ataque.\n", order[i] -> name);
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
      }
      else if(order[i] -> action == THREATEN){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	threaten(order[i], order[!i]);
      }
      else if(order[i] -> action == THROW_POISON){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	throw_poison(order[i], order[!i]);
      }
      else if(order[i] -> action == PARALYZE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	throw_paralysis(order[i], order[!i]);
      }
      else if(order[i] -> action == MAKE_SLEEP){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	make_sleep(order[i], order[!i]);
      }
      else if(order[i] -> action == CLEAN_EYES){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	order[i] -> blinded = false;
	order[i] -> onslaugh_penalty -= 1;
	printf("%s limpou seus olhos.\n", order[i] -> name);
      }
      else if(order[i] -> action == SMOKE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	smokescreen_move(order[i], order[!i]);
      }
      else if(order[i] -> action == FULL_DEFENSE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	order[i] -> full_defense = true;
	if(order[i] -> full_defense_move == protect)
	  printf("%s está se protegendo: +2 defesa, -2 Iniciativa.\n",
		 order[i] -> name);
	if(order[i] -> full_defense_move == detect)
	  printf("%s está detectando próximos ataques: +2 def, -2 Iniciativa.\n",
		 order[i] -> name);
	add_initiative(order[i], order[!i], -2, true);
      }
      else if(order[i] -> action == DISENGAGE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	disengage(order[i], order[!i]);
      }
      else if(order[i] -> action == ATT_DISENGAGE){
	attack_and_disengage(order[i], order[!i]);
      }
      else if(order[i] -> action == AIM_ATT){
	aim_att(order[i], order[!i]);
      }
      else if(order[i] -> action == HIDE_ATT){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);	
	hide_att(order[i], order[!i]);
      }
      else if(order[i] -> action == TAKE_COVER){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	take_cover(order[i]);
      }
      else if(order[i] -> action == DIF_TERRAIN){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	order[!i] -> difficult_terrain = true;
	printf("%s lançou %s e está difícil para %s se deslocar.\n",
	       order[i] -> name, order[i] -> dif_terrain_move, order[!i] -> name);
      }
      else if(order[i] -> action == RESTRAIN){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	printf("%s está impedindo ações de %s.\n", order[i] -> name,
	       order[!i] -> name);
	order[i] -> control_turns -= 2;
	order[!i] -> restrained = true;
      }
      else if(order[i] -> action == GRAPPLE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	grapple(order[i], order[!i]);
      }
      else if(order[i] -> action == SLAM){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	slam_att(order[i], order[!i]);
      }
      else if(order[i] -> action == GET_UP){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	printf("%s está tentando se levantar.\n", order[i] -> name);
	if(order[i] -> distance == 0 && order[!i] -> distance == 0){
	  if(roll(GET_ATTACK_DICE(order[i]) + GET_BONUS_MOVE(order[i])) >= 2){
	    printf("E conseguiu.\n");
	    order[i] -> prone = false;
	  }
	  else
	    printf("Mas não conseguiu com %s atrapalhando.\n", order[!i] -> name);
	}
	else
	  printf("Mas não conseguiu com %s atrapalhando.\n", order[!i] -> name);
      }
      else if(order[i] -> action == AMBUSH){
	ambush(order[i], order[!i]);
      }
      else if(order[i] -> action == SCARE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	inspire_fear(order[i], order[!i]);
      }
      else if(order[i] -> action == ANGER){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	inspire_anger(order[i], order[!i]);
      }
      else if(order[i] -> action == FRIENDSHIP){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	inspire_friendship(order[i], order[!i]);
      }
      else if(order[i] -> action == FLIRT){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	inspire_love(order[i], order[!i]);
      }
      else if(order[i] -> action == SEDUCE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	seduce(order[i], order[!i]);
      }
      else if(order[i] -> action == TRANSFORM){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	transform(order[i], order[!i]);
      }
      else if(order[i] -> action == BURN){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	throw_burn(order[i], order[!i]);
      }
      else if(order[i] -> action == END_BURN){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	end_burn(order[i]);
      }
      else if(order[i] -> action == NONE){
	if(order[!i] -> action == CLASH_WITHERING)
	  withering_attack(order[!i], order[i]);
	else if(order[!i] -> action == CLASH_DECISIVE)
	  decisive_attack(order[!i], order[i]);
	printf("Não há nada que %s possa fazer.\n", order[i] -> name);
      }
      if(order[!i] -> control_turns){
	order[!i] -> control_turns --;
	if(order[!i] -> control_turns == 0){
	  order[i]  -> grappled = false;
	  printf("%s escapou de '%s'.\n", order[i] -> name,
		 order[!i] -> grapple_move);
	}
      }
      if(order[i] -> extra_turn){
	order[i] -> aiming = 0;
	printf("%s ganhou uma ação extra!\n", order[i] -> name);
	order[i] -> extra_turn = false;
	i --;
      }
      GETCHAR();
    }
    pokemon_update_turn(p1, p2);
    pokemon_update_turn(p2, p1);
    ///////////
  } while(p1 -> damage <= p1 -> vitality && p2 -> damage <= p2 -> vitality &&
	  p1 -> forfeit == false && p2 -> forfeit == false && rodada < 100);
  if(test){
    struct pokemon *winner = NULL;
    if(p1 -> damage > p1 -> vitality || p1 -> forfeit)
      winner = p2;
    if(p2 -> damage > p2 -> vitality || p2 -> forfeit)
      winner = p1;
    if(winner != NULL)
       fprintf(stderr, "%s ganhou.\n", winner -> name);
    else
      fprintf(stderr, "EMPATE.\n");
  }
  free(p1);
  free(p2);
  _Wdestroy_rng(free, RNG);
  return 0;
}
