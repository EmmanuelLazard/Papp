/*
 * entrejou.c: entree d'un fullname de joueur
 *
 * (EL) 22/09/2012 : v1.36, no change.
 * (EL) 12/09/2012 : v1.35, no change
 * (EL) 16/07/2012 : v1.34, no change
 * (EL) 05/05/2008 : v1.33, Tous les 'int' deviennent 'long' pour etre sur d'etre sur 4 octets.
 * (EL) 21/04/2008 : v1.32 Separation fullname+prenom lors de l'entree d'un nouveau joueur.
 * (EL) 29/04/2007 : v1.31, no change
 * (EL) 19/03/2007 : Modification esthetique des chaines affichees (TAB).
 * (EL) 12/02/2007 : Modification de 'affiche_inscrits()', 'affiche_equipes()'
 *                   pour qu'ils affichent le fullname du tournoi au debut.
 * (EL) 02/02/2007 : 'tieBreak' devient un double
 * (EL) 13/01/2007 : v1.30 by E. Lazard, no change
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "global.h"
#include "joueur.h"
#include "more.h"



Players_list
        *registered_players   = NULL,
        *new_players   = NULL,
        *emigrant_players    = NULL,
        *team_captain = NULL;


#ifdef ENGLISH
#define BASE_JOUEURS_GROSSE  "I will not show you all the players in the database...\n" \
                             "Please type at least one letter before using the TAB\n"
#else
#define BASE_JOUEURS_GROSSE  "Je ne vais pas vous montrer toute la base des joueurs...\n"\
                             "Utilisez la touche TAB apres avoir tape au moins une lettre\n"
#endif


/*
 * Entree au clavier d'un fullname de joueur, avec affichage d'un prompt :
 * tampon_initial permet d'initialiser le tampon, et parmi_ces_joueurs
 * est la liste dans laquelle la fonction fait la completion quand
 * l'utilisateur tape [TAB] (passer NULL dans parmi_ces_joueurs pour
 * faire la completion dans toute la base).
 * Cette fonction reconnait les touches speciales suivantes:
 *
 *   ^C       vide le tampon, et quitte.
 *
 *   ^D,Tab   affiche toutes les completions possibles, puis complete
 *             (si possible) le tampon.
 *
 *   ^M,^J    quitte
 *
 *   Esc,^X   vide le tampon.
 *
 * La fonction retourne alors un pointeur sur le tampon. Celui-ci est
 * statique, et doit donc etre copie quelque part avant que la fonction ne
 * soit invoquee une nouvelle fois. La longueur de la chaine ne peut exceder
 * TAILLE_TAMPON caracteres (sans compter le \0 final).
 */

char *entree_clav_nom_joueur (const char *prompt, char *tampon_initial, Players_list *parmi_ces_joueurs) {
    long exitcode ;
    unsigned long l, i, nb_rep,  long_rep,  j, nbc, nbl;
    static char tampon[TAILLE_TAMPON+1];
    char c, d;
    static Players_list *liste_comp = NULL;



    if (tampon_initial == NULL)
       tampon[0] = '\0';
    else
       strcpy(tampon,tampon_initial);

    while (1) {
        printf("%s ", prompt);
        exitcode = lire_ligne_init(tampon, TAILLE_TAMPON, 1);

        putchar('\n');
        if (exitcode == 0) /* Ok */
            return tampon;
        if (exitcode < 0)
            return "";  /* avorte */

        /*
         * L'utilisateur a tape sur Tab: il faut chercher les
         * completions possibles du fullname
         */
        est_un_nom_de_joueur_valide(tampon);
        if (parmi_ces_joueurs == NULL) {
            if ( (strlen(tampon) == 0) && (nombre_joueurs_base() >= 100) ) {
                /* ne pas afficher toute la base des joueurs  :-/   */
                printf(BASE_JOUEURS_GROSSE);
                if (liste_comp==NULL)
                    liste_comp = creer_liste();
                else
                    vider_liste(liste_comp);
            } else
                liste_comp = recherche_nom(liste_comp, tampon);
        } else
            liste_comp = recherche_nom_dans_liste(liste_comp, tampon, parmi_ces_joueurs);

        /* Combien de reponses ? */
        nb_rep = liste_comp->n;
        if (nb_rep == 0) {
            /* Pas de completion: on laisse le tampon tel quel */
            beep();
            continue;
        }
        assert(nb_rep > 0);

        /* Determiner la longueur maximum des reponses */
        long_rep = 0;
        for (i = 0; i < nb_rep; i++) {
            assert(liste_comp->list[i]);
            l = strlen((liste_comp->list)[i]->fullname);
            if (l > long_rep)
                long_rep = l;
        }

        /* Combien peut-on en mettre par ligne ? */
        nbc = (nb_colonnes-1) / (long_rep+1);
        /* Mais nous voulons qu'il y ait assez de lignes */
        while (nbc>1 && 2*nbc*nbc>nb_rep)
            --nbc;
        nbl = (nb_rep + nbc - 1) / nbc;
        assert(nbc>0 && nbl>0);
        for (i=0; i<nbl; i++) {
            for (j=0; j<nbc && (l=i+nbl*j)<nb_rep; j++)
                printf(" %-*s", (int)long_rep,
                    (liste_comp->list)[l]->fullname);
            putchar('\n');
        }

        /* Completer le buffer */
        for(l = strlen(tampon); ; l++) {
            c = ((liste_comp->list)[0]->fullname)[l];
            if (c == '\0')
                goto b_comp;

            for (i=1; i<nb_rep; i++) {
                d = ((liste_comp->list)[i]->fullname)[l];
                if (tolower(c) != tolower(d))
                    goto b_comp;
            }
        }
b_comp:
        /* l contient la longueur de la plus grande completion */
        if (l > TAILLE_TAMPON)
            l = TAILLE_TAMPON;
        if (l >= strlen(tampon)) {
            strncpy(tampon, (liste_comp->list)[0]->fullname, l);
            tampon[l] = '\0';
            assert(tampon[TAILLE_TAMPON]==0);
        }
        /* Revenir au prompt */
    }
}

/*
 * Cette fonction est appelee par le programme principal, et invoque la
 * fonction entree_clav_nom_joueur() pour l'entree interactive des noms.
 * Elle est quittee des que l'utilisateur entre un fullname vide (par exemple
 * s'il tape ^C).
 */

#ifdef ENGLISH
# define ENTR_TAB   "Press [TAB] to complete a name\n\n"
# define ENTR_PROMPT    "Full name:"
# define ENTR_NELO  "Enter his/her ELO number:"
# define ENTR_NEWP  "Is this a new player (Y/N)? "
# define ENTR_NATION    "Which country? "
# define ENTR_PROP1 "Number %ld is free, "
# define ENTR_PROP2 "do you accept it (Y/N)? "
# define ENTR_CHOOSE    "Then choose another one:"
# define ENTR_AGAIN "This number is already allocated; please try again:"
# define ENTR_DUP   "This player has already been inscribed !"
# define LIST_HDR1  "There are %ld inscribed players:"
# define LIST_HDR2  "Ranking of the %ld players after round %ld:"
# define TEAM_HDR1  "There are %ld teams:"
# define TEAM_HDR2  "Ranking of the %ld teams after round %ld:"
#else
# define ENTR_TAB   "Appuyer sur [TAB] pour completer un nom\n\n"
# define ENTR_PROMPT    "Nom & prenom :"
# define ENTR_NELO  "Entrez le numero ELO du joueur :"
# define ENTR_NEWP  "Est-ce un nouveau joueur (O/N) ? "
# define ENTR_NATION    "Quelle est sa nationalite ? "
# define ENTR_PROP1 "Je vous propose le numero %ld, "
# define ENTR_PROP2 "etes-vous d'accord (O/N) ?"
# define ENTR_CHOOSE    "Alors veuillez m'en proposer un autre :"
# define ENTR_AGAIN "Ce numero est deja utilise, recommencez :"
# define ENTR_DUP   "Ce joueur est deja inscrit !"
# define LIST_HDR1  "Liste des %ld joueurs inscrits :"
# define LIST_HDR2  "Classement des %ld joueurs apres la ronde %ld :"
# define TEAM_HDR1  "Liste des %ld equipes :"
# define TEAM_HDR2  "Classement des %ld equipes apres la ronde %ld :"
#endif

void entree_joueurs() {
    static Players_list *lj=NULL;
    Player *j;
    char *nom_joueur, *pays, *tampon;
    long i, n;
	char *ptr_dernier_espace, *ptr_premier_espace ;
	char *prenom ;

    assert(registered_players != NULL);
    assert(new_players != NULL);
    assert(emigrant_players  != NULL);
    eff_ecran();
    inv_video(ENTR_TAB) ;
    pays = new_string();

    tampon = "";
    while ((nom_joueur = entree_clav_nom_joueur(ENTR_PROMPT, tampon , NULL))[0]) {
        tampon = "";
        if (!est_un_nom_de_joueur_valide(nom_joueur)) {
           beep();
           tampon = nom_joueur;
           continue;
        }

redo:
        /* Chercher si le joueur est dans la base */
        if (lj == NULL)
            lj = creer_liste();
        else
            vider_liste(lj);
        for (j = premier_joueur(); j; j=j->next)
            if (!compare_chaines_non_sentitif(j->fullname, nom_joueur)) {
                ajouter_joueur(lj, j);
                printf(j->comment ?
                    "%ld\t%s {%s} -- %s\n" : "%ld\t%s {%s}\n",
                    j->ID,j->fullname,j->country,j->comment);
            }
        /* Combien de reponses ? */
        if (lj->n == 1)
            j = (lj->list)[0];
        if (lj->n > 1)
            do {
                /* Il y a ambiguite, nous demandons le ID ELO */
                printf(ENTR_NELO);
                i = sscanf(lire_ligne(),"%ld", &n); putchar('\n');
                if (i < 1)
                    goto redo;
                j = NULL;
                for (i=0; i<lj->n; i++)
                    if ((lj->list)[i]->ID == n)
                        j = (lj->list)[i];
            } while (j==NULL);
        if (lj->n == 0) {
            /* Est-ce un nouveau joueur ou une erreur? */
            if (oui_non(ENTR_NEWP)==0) {
                tampon = nom_joueur;
                continue;
            }

           /* Nationalite */
entree_pays:
            printf(ENTR_NATION);
            strcpy(pays,lire_ligne());
            enleve_espaces_de_gauche(pays);
            if (pays[0] == 0)
                puts(strcpy(pays, pays_defaut));
            else
            putchar('\n');
            enleve_espaces_de_gauche(pays);
            if (pays[0] == 0) {
               beep();
               goto entree_pays;
            }

            /* Il faut choisir un ID Elo */
            n = inserer_joueur(pays);


            /* Modification par Stephane Nicolet : nous ne demandons
               plus confirmation a l'utilisateur du choix par Papp
               d'un nouveau ID : je crois que cette fonction
               n'etait _jamais_ utilisee et que l'on acceptait
               _toujours_ le ID par defaut. Est-ce que je me
               trompe ?
            */
            /*
            printf(ENTR_PROP1, n);
            if ((oui_non(ENTR_PROP2) == 0)) {
              printf(ENTR_CHOOSE);
              for(;;) {
            i = sscanf(lire_ligne(), "%ld", &n); putchar('\n');
            if (i < 1 || n <= 0)
                {beep(); goto redo;}
            if (trouver_joueur(n))
              printf(ENTR_AGAIN);
            else
              break;
              }
            }
            */


            /* Creer un nouveau joueur */
            assert(nom_joueur && nom_joueur[0]);
			/* On decoupe le fullname complet en fullname + prenom (la derniere chaine sans espace) */
			ptr_dernier_espace = strrchr(nom_joueur, ' ') ;
			ptr_premier_espace =  strchr(nom_joueur, ' ') ;
			if ((ptr_dernier_espace != NULL) && (ptr_premier_espace != nom_joueur)) {
				COPIER(ptr_dernier_espace+1, &prenom) ;
				*ptr_dernier_espace = '\0' ; /* On coupe */
			} else {
				prenom = NULL ;
			}
            j = nv_joueur(n, nom_joueur, prenom, NULL, pays, 0, NULL, 1);
            assert(n == j->ID);
            ajouter_joueur(new_players, j);
        }
        /* Inserer le joueur dans la liste des joueurs inscrits; nous
         * devons d'abord verifier qu'il n'y est pas deja; ce sera
         * automatiquement le cas pour les nouveaux joueurs
         */
        if (numero_inscription(j->ID)<0)    /* pas inscrit */
            inscrire_joueur(j->ID);
        else {
            puts(ENTR_DUP);  /* deja inscrit */
            beep();
            printf("\n");
        }
    } /* while */
}

/*
 * Ajoute un joueur de ID Elo 'ID' a la liste des joueurs inscrits,
 * et renvoie un ID d'inscription (le precedent ID d'inscription si
 * le joueur etait deja inscrit). Renvoie -1 si le joueur n'est pas dans
 * la base.
 */

long inscrire_joueur(long numero) {
    Player *j;
    long n;

    n = numero_inscription(numero);
    if (n >= 0) {
        assert(0 <= n && n < MAX_REGISTERED);
        present[n]   = 1;       /* Le joueur est present */
        return n;
    }
    if (n < 0)  {
        j = trouver_joueur(numero);
        if (j == NULL)
            return -1;      /* le joueur n'est pas dans la base */
        ajouter_joueur(registered_players, j);
        n = numero_inscription(numero);
    }

    assert(0 <= n && n < MAX_REGISTERED);
    present[n]   = 1;           /* Le joueur est present */
    score[n]     = 0;           /* zero point            */
    nbr_discs[n]  = ZERO_DISC;   /* zero pion             */
    tieBreak[n] = 0.0;         /* on ne sait jamais     */
    last_float[n] = 0;           /* il n'a pas flotte     */
    return n;
}

/*
 * Important : cet ordre de tri doit etre le meme que celui implemente dans
 * la fonction compar() de crosstable.c
 */
int tri_joueurs(const void *ARG1, const void *ARG2) {
	long *n1 = (long *) ARG1 ;
 	long *n2 = (long *) ARG2 ;
    long d, j1=(*n1), j2=(*n2);

    if ((d = score[j2] - score[j1]) != 0)
        return d;
/*  if (DIFFERENT_SCORES(tieBreak[j2],tieBreak[j1])) {
        if SCORE_IS_LARGER(tieBreak[j2],tieBreak[j1])
            return 1;
        if SCORE_IS_SMALLER(tieBreak[j2],tieBreak[j1])
            return -1;
    } */
    if (tieBreak[j2] != tieBreak[j1]) {
        if (tieBreak[j2] > tieBreak[j1])
            return 1 ;
        if (tieBreak[j2] < tieBreak[j1])
            return -1 ;
    }
    if (DIFFERENT_SCORES(nbr_discs[j2],nbr_discs[j1])) {
        if SCORE_IS_LARGER(nbr_discs[j2],nbr_discs[j1])
            return 1;
        if SCORE_IS_SMALLER(nbr_discs[j2],nbr_discs[j1])
            return -1;
    }
    return compare_chaines_non_sentitif(registered_players->list[j1]->fullname,
                                        registered_players->list[j2]->fullname);
}

static int tri_equipes(const void *ARG1, const void *ARG2) {
	long *n1 = (long *) ARG1 ;
 	long *n2 = (long *) ARG2 ;
    long d, j1=(*n1), j2=(*n2);

    if ((d = team_score[j2] - team_score[j1]) != 0)
        return d;
    else
        return compare_chaines_non_sentitif(team_captain->list[j1]->country,
                                            team_captain->list[j2]->country);
}

/*
 * Affichage de la liste des joueurs inscrits
 */
void affiche_inscrits(const char *filename) {
    long i, _i, nbi, nulles, len_dep;
    char chaine[256], chaine2[256];
    char *nomTrn ;

    assert(registered_players != NULL);
    assert(new_players != NULL);
    assert(emigrant_players  != NULL);

    /* Peut-etre devons-nous sauvegarder l'etat */
    if (sauvegarde_immediate)
        sauve_inscrits();
    nbi = registered_players->n;
    calcul_departage();

    more_init(filename);

    /* Pour afficher le fullname du tournoi en tete */
    nomTrn = malloc(strlen(nom_du_tournoi)+15) ;
    if (nomTrn != NULL) {
        sprintf(nomTrn, "*** %s ***\n", nom_du_tournoi) ;
        more_line(nomTrn) ;
        free(nomTrn) ;
    }

    sprintf(chaine, current_round? LIST_HDR2 : LIST_HDR1, nbi, current_round);
    more_line(chaine);
    more_line("");          /* ligne blanche */

    if (nbi > 0) {
        long    table[MAX_REGISTERED];
        long    dernier_score = -1;
        double  dernier_departage = -1.0;
        Player *j;

        for (i=0; i<nbi; i++)
            table[i] = i;
        SORT(table, nbi, sizeof(long), tri_joueurs);

        /* y a-t-il des nulles a afficher ? */
        nulles = 0; len_dep = 0;
        for (i=0; i<nbi; i++) {
            nulles |= (score[table[i]] % 2);
            len_dep = le_max_de(len_dep, strlen(departage_en_chaine(tieBreak[i])));
        }

        /* Afficher la liste triee */
        for (i=0; i<nbi; i++) {
            _i = table[i];
            assert(_i >= 0 && _i < nbi);
            j  = registered_players->list[_i];

            sprintf(chaine2, "[%s]", departage_en_chaine(tieBreak[_i]));
            sprintf(chaine, "%*s", (int)len_dep + 2, chaine2);

            /* Meme score que le joueur precedent? */
            if (dernier_score != score[_i]) {
                if (score[_i] >= 3) {
                    if ((score[_i] % 2) == 1)
                        sprintf(chaine2,"%3ld:%4ld.5 pts %s", i+1, (score[_i] / 2 ), chaine);
                    else
                        if (nulles)
                            sprintf(chaine2,"%3ld:%4ld.  pts %s", i+1, (score[_i] / 2 ), chaine);
                        else
                            sprintf(chaine2,"%3ld:%4ld pts %s", i+1, (score[_i] / 2 ), chaine);
                }
                else {
                    if ((score[_i] % 2) == 1)
                        sprintf(chaine2,"%3ld:%4ld.5 pt  %s", i+1, (score[_i] / 2 ), chaine);
                    else
                        if (nulles)
                            sprintf(chaine2,"%3ld:%4ld.  pt  %s", i+1, (score[_i] / 2 ), chaine);
                        else
                            sprintf(chaine2,"%3ld:%4ld pt  %s", i+1, (score[_i] / 2 ), chaine);
                }
            }
            else {
                if (dernier_departage == tieBreak[_i])
                    sprintf(chaine, "%*s", (int)len_dep + 2, "");
                if (nulles)
                    sprintf(chaine2, "%14s %s", "", chaine);
                else
                    sprintf(chaine2, "%12s %s", "", chaine);
            }

            dernier_score = score[_i];
            dernier_departage = tieBreak[_i];

            /* Nom, prenom, ID, pays... */
            sprintf(chaine,
              compare_chaines_non_sentitif(j->country, pays_defaut)==0 ?
              "%s  %c%s (%ld)" : "%s  %c%s (%ld) {%s}", chaine2,
              present[_i]? ' ':'-', j->fullname,
              j->ID, j->country);
            more_line(chaine);
        }
    }   /* if nbi>0 */
    more_close();
}

void  affiche_equipes(const char *filename) {
    long i, i_ , nbi, nb_equipes, trouve;
    char chaine[256];
    long equipes[MAX_REGISTERED];
    char *nomTrn ;

    /* Peut-etre devons-nous sauvegarder l'etat */
    if (sauvegarde_immediate)
        sauve_inscrits();
    nbi = registered_players->n;
    calcul_departage();

    /* Pour chaque equipe presente, on garde une reference sur  */
    /* l'un des representants de cette equipe : le "capitaine". */
    /* Cela permet d'acceder facilement au fullname de l'equipe pour */
    /* les tris et les affichages */
    if (team_captain == NULL)
        team_captain = creer_liste();
    else
        vider_liste(team_captain);

    assert(registered_players != NULL);
    assert(team_captain != NULL);

    /* Ici, le capitaine de l'equipe nationale sera simplement */
    /* le premier joueur inscrit de ce pays */
    nb_equipes = 0;
    for (i = 0; i < registered_players->n; i++) {
        trouve = 0;
        for (i_ = 0; i_ < team_captain->n; i_++)
            if (!compare_chaines_non_sentitif(registered_players->list[i]->country,
                                              team_captain->list[i_]->country)) {
                trouve = 1;
                team_score[i_] += score[i];
            }
        if (!trouve) {
            ajouter_joueur(team_captain, registered_players->list[i]);
            team_score[nb_equipes] = score[i];
            nb_equipes++;
        }
    }


    more_init(filename);

    /* Pour afficher le fullname du tournoi en tete */
    nomTrn = malloc(strlen(nom_du_tournoi)+15) ;
    if (nomTrn != NULL) {
        sprintf(nomTrn, "*** %s ***\n", nom_du_tournoi) ;
        more_line(nomTrn) ;
        free(nomTrn) ;
    }

    sprintf(chaine, current_round? TEAM_HDR2 : TEAM_HDR1, nb_equipes, current_round);
    more_line(chaine);
    more_line("");          /* ligne blanche */

    if (nb_equipes > 0) {

        for (i=0; i<nb_equipes; i++)
            equipes[i] = i;
        SORT(equipes, nb_equipes, sizeof(long), tri_equipes);

        for (i=0; i<nb_equipes; i++) {
            i_ = equipes[i];
            assert(i_ >= 0 && i_ < nb_equipes);

            /* Nom du pays, score de l'equipe... */
            if ((team_score[i_] % 2) == 1)
                sprintf(chaine, "  %-7s  %5ld.5 pts",
                            team_captain->list[i_]->country,
                            team_score[i_]/2);
            else
                sprintf(chaine, "  %-7s  %5ld	pts",
                            team_captain->list[i_]->country,
                            team_score[i_]/2);

            more_line(chaine);
        }

    }   /* if nb_equipes>0 */
    more_close();

    vider_liste(team_captain);
}

/*
 * Renvoie le ID d'inscription du joueur de ID Elo 'n_elo', ou -1
 * si le joueur n'est pas encore inscrit.
 */

long numero_inscription(long n_elo) {
    long i;

    if (!n_elo)
        return -1 ;
    for (i=0; i<registered_players->n; i++)
    if (registered_players->list[i]->ID == n_elo)
        return i;
    /* pas trouve */
    return -1;
}

/*
 * Modifie la nationalite d'un joueur de ID Elo 'ID'.
 * Renvoie -1 si le joueur n'est pas dans la base.
 */

long change_nationalite (long numero, const char *pays) {
    Player *j;

    j = trouver_joueur (numero);
    if (j == NULL)
        return -1;
    if (compare_chaines_non_sentitif(j->country,pays))
        COPIER(pays, &j->country);
    return numero;
}
