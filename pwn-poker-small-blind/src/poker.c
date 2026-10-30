/*
 * Simple Texas Hold'em Poker (Singleplayer vs Dealer)
 *
 * Blinds:  By default, Player posts small blind, Dealer posts big blind.
 *          Compile with -DDEALER_POSTS_SMALL_BLIND to reverse this
 *          (Dealer = small blind, Player = big blind).
 * Rounds:  Pre-flop → Flop → Turn → River → Showdown
 * Actions: raise <n> / call / check / fold
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DECK_SIZE      52
#define HAND_SIZE      2
#define COMMUNITY_SIZE 5
#define SMALL_BLIND    10
#define BIG_BLIND      20
#define STARTING_CHIPS 500

static char flag[64] = "utflag{PLACEHOLDER}";
static char player_name[64];

/* Blinds and first-to-act alternate each hand.
 * On even-numbered hands (0, 2, 4, ...): player = small blind, acts first post-flop.
 * On odd-numbered hands  (1, 3, 5, ...): dealer = small blind, player = big blind,
 *                                        dealer acts first post-flop. */

typedef enum {
	CLUBS,
	DIAMONDS,
	HEARTS,
	SPADES,
} Suit;

#define FIRST_SUIT CLUBS
#define LAST_SUIT  SPADES

typedef struct {
	// 2-14 (11=J, 12=Q, 13=K, 14=A)
	int rank;
	Suit suit;
} Card;

typedef struct {
	Card cards[DECK_SIZE];
	int top;
} Deck;

typedef enum {
	HIGH_CARD = 1,
	ONE_PAIR,
	TWO_PAIR,
	THREE_OF_A_KIND,
	STRAIGHT,
	FLUSH,
	FULL_HOUSE,
	FOUR_OF_A_KIND,
	STRAIGHT_FLUSH,
	ROYAL_FLUSH
} HandRank;

#define NULL_HAND_RANK ((HandRank) 0)
#define MIN_HAND_RANK  HIGH_CARD
#define MAX_HAND_RANK  ROYAL_FLUSH

typedef struct {
	HandRank rank;
	int tiebreakers[5];
} HandValue;

typedef enum {
	PLAYER_FOLD,
	DEALER_FOLD,
	BETTING_COMPLETE
} BettingResult;

static const char *suit_str(Suit s, bool fancy) {
	switch (s) {
		case CLUBS:
			return fancy ? "♣" : "C";
		case DIAMONDS:
			return fancy ? "♦" : "D";
		case HEARTS:
			return fancy ? "♥" : "H";
		case SPADES:
			return fancy ? "♠" : "S";
		default:
			return "?";
	}
}

static const char *rank_str(int r) {
	switch (r) {
		case 2:
			return "2";
		case 3:
			return "3";
		case 4:
			return "4";
		case 5:
			return "5";
		case 6:
			return "6";
		case 7:
			return "7";
		case 8:
			return "8";
		case 9:
			return "9";
		case 10:
			return "10";
		case 11:
			return "J";
		case 12:
			return "Q";
		case 13:
			return "K";
		case 14:
			return "A";
		default:
			return "?";
	}
}

static const char *hand_rank_str(HandRank hr) {
	switch (hr) {
		case HIGH_CARD:
			return "High Card";
		case ONE_PAIR:
			return "One Pair";
		case TWO_PAIR:
			return "Two Pair";
		case THREE_OF_A_KIND:
			return "Three of a Kind";
		case STRAIGHT:
			return "Straight";
		case FLUSH:
			return "Flush";
		case FULL_HOUSE:
			return "Full House";
		case FOUR_OF_A_KIND:
			return "Four of a Kind";
		case STRAIGHT_FLUSH:
			return "Straight Flush";
		case ROYAL_FLUSH:
			return "Royal Flush";
		default:
			return "Unknown";
	}
}

static void print_card(Card c, bool fancy) {
	printf("[%s%s]", rank_str(c.rank), suit_str(c.suit, fancy));
}

static void print_cards(const Card *cards, int n, bool fancy) {
	for (int i = 0; i < n; i++) {
		print_card(cards[i], fancy);
		if (i < n - 1) {
			printf(" ");
		}
	}
}

static void init_deck(Deck *d) {
	int i = 0;
	for (Suit s = FIRST_SUIT; s <= LAST_SUIT; s++) {
		for (int r = 2; r <= 14; r++) {
			d->cards[i++] = (Card) {r, s};
		}
	}
	d->top = 0;
}

static void shuffle_deck(Deck *d) {
	for (int i = DECK_SIZE - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		Card temp = d->cards[i];
		d->cards[i] = d->cards[j];
		d->cards[j] = temp;
	}
	d->top = 0;
}

static Card deal_card(Deck *d) {
	return d->cards[d->top++];
}

/* Sort 5 ranks descending */
static void sort_desc(int *arr, int n) {
	for (int i = 0; i < n - 1; i++) {
		for (int j = i + 1; j < n; j++) {
			if (arr[j] > arr[i]) {
				int temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
		}
	}
}

/*
 * Evaluate the value of a 5-card hand
 */
static HandValue eval5(const Card *five) {
	// flush
	int is_flush = 1;
	for (int i = 1; i < 5; i++) {
		if (five[i].suit != five[0].suit) {
			is_flush = 0;
			break;
		}
	}

	// straight
	int straight_ranks[5];
	for (int i = 0; i < 5; i++) {
		straight_ranks[i] = five[i].rank;
	}
	sort_desc(straight_ranks, 5);
	int is_straight = (straight_ranks[0] - straight_ranks[4] == 4
	                   && straight_ranks[0] != straight_ranks[1]
	                   && straight_ranks[1] != straight_ranks[2]
	                   && straight_ranks[2] != straight_ranks[3]
	                   && straight_ranks[3] != straight_ranks[4]);

	// rank counts
	int rank_counts[15] = {0};
	for (int i = 0; i < 5; i++) {
		rank_counts[five[i].rank]++;
	}

	int num_pairs = 0, num_triples = 0, num_quads = 0;
	int pair_ranks[2] = {0}, trip_rank = 0, quad_rank = 0;
	for (int r = 14; r >= 2; r--) {
		if (rank_counts[r] == 4) {
			num_quads++;
			quad_rank = r;
		}
		if (rank_counts[r] == 3) {
			num_triples++;
			trip_rank = r;
		}
		if (rank_counts[r] == 2) {
			assert(num_pairs < 2);
			pair_ranks[num_pairs] = r;
			num_pairs++;
		}
	}

	// kickers
	int kickers[5];
	int num_kickers = 0;
	for (int r = 14; r >= 2; r--) {
		if (rank_counts[r] == 1) {
			kickers[num_kickers++] = r;
		}
	}

	HandValue hand_value = {
	      NULL_HAND_RANK, {0, 0, 0, 0, 0}
    };

	if (is_straight && is_flush) {
		if (straight_ranks[0] == 14) {
			hand_value.rank = ROYAL_FLUSH;
			hand_value.tiebreakers[0] = 14;
		} else {
			hand_value.rank = STRAIGHT_FLUSH;
			hand_value.tiebreakers[0] = straight_ranks[0];
		}
	} else if (num_quads) {
		hand_value.rank = FOUR_OF_A_KIND;
		hand_value.tiebreakers[0] = quad_rank;
		hand_value.tiebreakers[1] = kickers[0];
	} else if (num_triples && num_pairs) {
		hand_value.rank = FULL_HOUSE;
		hand_value.tiebreakers[0] = trip_rank;
		hand_value.tiebreakers[1] = pair_ranks[0];
	} else if (is_flush) {
		hand_value.rank = FLUSH;
		for (int i = 0; i < 5; i++) {
			hand_value.tiebreakers[i] = straight_ranks[i];
		}
	} else if (is_straight) {
		hand_value.rank = STRAIGHT;
		hand_value.tiebreakers[0] = straight_ranks[0];
	} else if (num_triples) {
		hand_value.rank = THREE_OF_A_KIND;
		hand_value.tiebreakers[0] = trip_rank;
		for (int i = 0; i < 2; i++) {
			hand_value.tiebreakers[i + 1] = kickers[i];
		}
	} else if (num_pairs == 2) {
		hand_value.rank = TWO_PAIR;
		hand_value.tiebreakers[0] = pair_ranks[0];
		hand_value.tiebreakers[1] = pair_ranks[1];
		hand_value.tiebreakers[2] = kickers[0];
	} else if (num_pairs == 1) {
		hand_value.rank = ONE_PAIR;
		hand_value.tiebreakers[0] = pair_ranks[0];
		for (int i = 0; i < 3; i++) {
			hand_value.tiebreakers[i + 1] = kickers[i];
		}
	} else {
		hand_value.rank = HIGH_CARD;
		for (int i = 0; i < 5; i++) {
			hand_value.tiebreakers[i] = straight_ranks[i];
		}
	}
	return hand_value;
}

static int cmp_hand(HandValue a, HandValue b) {
	if (a.rank != b.rank) {
		return (int) a.rank - (int) b.rank;
	}
	for (int i = 0; i < 5; i++) {
		if (a.tiebreakers[i] != b.tiebreakers[i]) {
			return a.tiebreakers[i] - b.tiebreakers[i];
		}
	}
	return 0;
}

static HandValue find_best_hand(const Card *cards, int n) {
	HandValue best = {(HandRank) 0, {0}};
	// n choose 5
	for (int a = 0; a < n - 4; a++) {
		for (int b = a + 1; b < n - 3; b++) {
			for (int c = b + 1; c < n - 2; c++) {
				for (int d = c + 1; d < n - 1; d++) {
					for (int e = d + 1; e < n; e++) {
						Card five[5] = {cards[a], cards[b], cards[c], cards[d], cards[e]};
						HandValue hand_value = eval5(five);
						if (cmp_hand(hand_value, best) > 0) {
							best = hand_value;
						}
					}
				}
			}
		}
	}
	return best;
}

/// @brief Run a betting round for the given street. Player acts first, then dealer
/// responds.
/// @param street Name of the current street (e.g. "Pre-flop", "Flop", etc.) for display
/// purposes
/// @param player_chips Pointer to the player's chip count
/// @param dealer_chips Pointer to the dealer's chip count
/// @param pot Pointer to the pot size
/// @param player_bet The amount the player has bet this round
/// @param dealer_bet The amount the dealer has bet this round
/// @param player_to_act_first
/// @return PLAYER_FOLD or BETTING_COMPLETE
static BettingResult player_act(int *player_chips, int *dealer_chips, int *pot,
      int *player_bet, const int *dealer_bet) {
	char action[64];

	while (1) {
		int player_owes = *dealer_bet - *player_bet;

		if (player_owes > 0) {
			printf("Action (call %d / raise <n> / fold): ", player_owes);
		} else {
			printf("Action (check / raise <n> / fold): ");
		}

		if (fgets(action, sizeof(action), stdin) == NULL) {
			printf("Input error, folding.\n");
			*dealer_chips += *pot;
			*pot = 0;
			return PLAYER_FOLD;
		}
		action[strcspn(action, "\r\n")] = '\0';

		if (strncmp(action, "fold", 4) == 0) {
			printf("You folded. Dealer wins the pot.\n");
			*dealer_chips += *pot;
			*pot = 0;
			return PLAYER_FOLD;

		} else if (strncmp(action, "check", 5) == 0) {
			if (player_owes > 0) {
				printf("You can't check, as there's a bet of %d to you. "
				       "Try call, raise, or fold.\n",
				      player_owes);
				continue;
			}
			return BETTING_COMPLETE;

		} else if (strncmp(action, "call", 4) == 0) {
			if (player_owes <= 0) {
				printf("Nothing to call. Try check, raise, or fold.\n");
				continue;
			}
			int actual = (player_owes > *player_chips) ? *player_chips : player_owes;
			*player_chips -= actual;
			*pot += actual;
			*player_bet += actual;
			printf("You call %d.\n", actual);
			return BETTING_COMPLETE;

		} else if (strncmp(action, "raise", 5) == 0) {
			int amount = 0;
			char *sp = strchr(action, ' ');
			if (sp) {
				amount = atoi(sp + 1);
			}
			if (amount <= 0) {
				printf("Usage: raise <amount> (must be a multiple of %d)\n", BIG_BLIND);
				continue;
			}

			amount = (amount / BIG_BLIND) * BIG_BLIND;
			int min_raise = player_owes + BIG_BLIND;
			if (amount < min_raise) {
				printf("Minimum raise is %d (call %d + one blind %d). Amounts are "
				       "rounded down to multiples of %d.\n",
				      min_raise, player_owes, BIG_BLIND, BIG_BLIND);
				continue;
			}
			if (amount > *player_chips) {
				printf("You only have %d chips.\n", *player_chips);
				continue;
			}
			*player_chips -= amount;
			*pot += amount;
			*player_bet += amount;
			printf("You raise %d.\n", amount);
			return BETTING_COMPLETE;
		} else {
			printf("Unknown action. Try: check / raise <n> / call / fold\n");
		}
	}
}

/// @brief Dealer opens the betting round.
/// @param player_chips Pointer to the player's chip count
/// @param dealer_chips Pointer to the dealer's chip count
/// @param pot Pointer to the pot size
/// @param player_bet Pointer to the player's bet amount
/// @param dealer_bet Pointer to the dealer's bet amount
/// @return PLAYER_FOLD or BETTING_COMPLETE
static BettingResult dealer_open(int *player_chips, int *dealer_chips, int *pot,
      int *player_bet, int *dealer_bet) {
	int r = rand() % 100;
	if (r < 30 && *dealer_chips >= BIG_BLIND) {
		// 30%: dealer bets a random amount
		int amount = BIG_BLIND + (rand() % (BIG_BLIND * 3));
		amount = (amount / BIG_BLIND) * BIG_BLIND;
		if (amount < BIG_BLIND) {
			amount = BIG_BLIND;
		}
		if (amount > *dealer_chips) {
			amount = (*dealer_chips / BIG_BLIND) * BIG_BLIND;
			if (amount < BIG_BLIND) {
				amount = *dealer_chips;
			}
		}
		*dealer_chips -= amount;
		*pot += amount;
		*dealer_bet += amount;
		printf("Dealer bets %d.\n", amount);
	} else {
		// 70%: dealer checks
		printf("Dealer checks.\n");
	}

	BettingResult response
	      = player_act(player_chips, dealer_chips, pot, player_bet, dealer_bet);
	if (response == PLAYER_FOLD) {
		return response;
	}

	// Dealer calls any raise the player made
	int d_call = *player_bet - *dealer_bet;
	if (d_call > 0) {
		if (d_call > *dealer_chips) {
			d_call = *dealer_chips;
		}
		*dealer_chips -= d_call;
		*pot += d_call;
		*dealer_bet += d_call;
		printf("Dealer calls %d.\n", d_call);
	}
	return BETTING_COMPLETE;
}

/// @brief Dealer responds to a player's action.
/// @param player_chips Pointer to the player's chip count
/// @param dealer_chips Pointer to the dealer's chip count
/// @param pot Pointer to the pot size
/// @param player_bet Pointer to the player's bet amount
/// @param dealer_bet Pointer to the dealer's bet amount
/// @return PLAYER_FOLD, DEALER_FOLD, or BETTING_COMPLETE
static BettingResult dealer_respond(int *player_chips, int *dealer_chips, int *pot,
      int *player_bet, int *dealer_bet) {
	int dealer_to_call = *player_bet - *dealer_bet;
	int r = rand() % 100;
	if (dealer_to_call <= 0) {
		if (r < 30 && *dealer_chips >= BIG_BLIND) {
			// 30%: dealer raises a random amount
			int amount = BIG_BLIND + (rand() % (BIG_BLIND * 3));
			amount = (amount / BIG_BLIND) * BIG_BLIND;
			if (amount < BIG_BLIND) {
				amount = BIG_BLIND;
			}
			if (amount > *dealer_chips) {
				amount = (*dealer_chips / BIG_BLIND) * BIG_BLIND;
				if (amount < BIG_BLIND) {
					amount = *dealer_chips;
				}
			}
			*dealer_chips -= amount;
			*pot += amount;
			*dealer_bet += amount;
			printf("Dealer raises %d.\n", amount);

			BettingResult response
			      = player_act(player_chips, dealer_chips, pot, player_bet, dealer_bet);
			if (response == PLAYER_FOLD) {
				return response;
			}

			// dealer calls any re-raise
			int d_call = *player_bet - *dealer_bet;
			if (d_call > 0) {
				if (d_call > *dealer_chips) {
					d_call = *dealer_chips;
				}
				*dealer_chips -= d_call;
				*pot += d_call;
				*dealer_bet += d_call;
				printf("Dealer calls %d.\n", d_call);
			}
		} else {
			// 70%: dealer checks
			printf("Dealer checks.\n");
		}
	} else {
		if (r < 5) {
			// 5%: dealer folds
			printf("Dealer folds! You win the pot of %d chips!\n", *pot);
			*player_chips += *pot;
			*pot = 0;
			return DEALER_FOLD;
		} else if (r < 30 && *dealer_chips >= dealer_to_call + BIG_BLIND) {
			// 25%: dealer raises a random amount
			int raise_extra = BIG_BLIND + (rand() % (BIG_BLIND * 2));
			raise_extra = (raise_extra / BIG_BLIND) * BIG_BLIND;
			if (raise_extra < BIG_BLIND) {
				raise_extra = BIG_BLIND;
			}
			int total = dealer_to_call + raise_extra;
			if (total > *dealer_chips) {
				raise_extra = ((*dealer_chips - dealer_to_call) / BIG_BLIND) * BIG_BLIND;
				if (raise_extra < BIG_BLIND) {
					raise_extra = BIG_BLIND;
				}
				total = dealer_to_call + raise_extra;
			}
			*dealer_chips -= total;
			*pot += total;
			*dealer_bet += total;
			printf("Dealer raises by %d (total %d).\n", raise_extra, total);

			BettingResult response
			      = player_act(player_chips, dealer_chips, pot, player_bet, dealer_bet);
			if (response != BETTING_COMPLETE) {
				return response;
			}

			// dealer calls any re-raise
			int d_call = *player_bet - *dealer_bet;
			if (d_call > 0) {
				if (d_call > *dealer_chips) {
					d_call = *dealer_chips;
				}
				*dealer_chips -= d_call;
				*pot += d_call;
				*dealer_bet += d_call;
				printf("Dealer calls %d.\n", d_call);
			}
		} else {
			// 70%: dealer calls
			int actual
			      = (dealer_to_call > *dealer_chips) ? *dealer_chips : dealer_to_call;
			*dealer_chips -= actual;
			*pot += actual;
			*dealer_bet += actual;
			printf("Dealer calls %d.\n", actual);
		}
	}
	return BETTING_COMPLETE;
}

/// @brief Run a betting round for the given street.
/// @param street Name of the current street (e.g. "Pre-flop", "Flop", etc.)
/// @param player_chips Pointer to the player's chip count
/// @param dealer_chips Pointer to the dealer's chip count
/// @param pot Pointer to the pot size
/// @param player_bet The amount the player has already bet this hand
/// @param dealer_bet The amount the dealer has already bet this hand
/// @param player_acts_first true if the player opens the round, false if dealer opens
/// @return PLAYER_FOLD, DEALER_FOLD, or BETTING_COMPLETE
static BettingResult betting_round(const char *street, int *player_chips,
      int *dealer_chips, int *pot, int player_bet, int dealer_bet,
      bool player_acts_first) {
	printf("\n── %s ──\n", street);
	printf("Pot: %d chips | Your chips: %d | Dealer chips: %d\n", *pot, *player_chips,
	      *dealer_chips);
	printf("Current bets this round — You: %d | Dealer: %d\n", player_bet, dealer_bet);

	if (player_acts_first) {
		BettingResult r
		      = player_act(player_chips, dealer_chips, pot, &player_bet, &dealer_bet);
		if (r == PLAYER_FOLD) {
			return r;
		}
		return dealer_respond(player_chips, dealer_chips, pot, &player_bet, &dealer_bet);
	} else {
		return dealer_open(player_chips, dealer_chips, pot, &player_bet, &dealer_bet);
	}
}

/// @brief Handles the showdown phase: evaluates both player's and dealer's hands,
/// determines the winner, and awards the pot accordingly.
/// @param player_hole The player's hole cards.
/// @param dealer_hole The dealer's hole cards.
/// @param community The community cards.
/// @param n_community The number of community cards.
/// @param player_chips Pointer to the player's chip count.
/// @param dealer_chips Pointer to the dealer's chip count.
/// @param pot Pointer to the pot size.
/// @param fancy_printing Whether to use fancy printing for the cards.
static void showdown(const Card *player_hole, const Card *dealer_hole,
      const Card *community, int n_community, int *player_chips, int *dealer_chips,
      int *pot, bool fancy_printing) {
	printf("\n══════════════ SHOWDOWN ══════════════\n");
	printf("Your hole cards:   ");
	print_cards(player_hole, 2, fancy_printing);
	printf("\n");
	printf("Dealer hole cards: ");
	print_cards(dealer_hole, 2, fancy_printing);
	printf("\n");
	printf("Community:         ");
	print_cards(community, n_community, fancy_printing);
	printf("\n");

	// build full 7-card hands
	Card player_all[7], dealer_all[7];
	memcpy(player_all, player_hole, 2 * sizeof(Card));
	memcpy(player_all + 2, community, n_community * sizeof(Card));
	memcpy(dealer_all, dealer_hole, 2 * sizeof(Card));
	memcpy(dealer_all + 2, community, n_community * sizeof(Card));

	HandValue player_value = find_best_hand(player_all, 2 + n_community);
	HandValue dealer_value = find_best_hand(dealer_all, 2 + n_community);

	printf("\nYour best hand:   %s\n", hand_rank_str(player_value.rank));
	printf("Dealer best hand: %s\n", hand_rank_str(dealer_value.rank));

	int cmp = cmp_hand(player_value, dealer_value);
	if (cmp > 0) {
		printf("You win the pot of %d chips!\n", *pot);
		*player_chips += *pot;
	} else if (cmp < 0) {
		printf("Dealer wins the pot of %d chips.\n", *pot);
		*dealer_chips += *pot;
	} else {
		printf("It's a tie! Pot split.\n");
		*player_chips += *pot / 2;
		*dealer_chips += *pot - *pot / 2;
	}
	*pot = 0;
}

/// @brief Prints the player's hand, including hole cards and best hand rank.
/// @param hole The player's hole cards.
/// @param community The community cards.
/// @param n_community The number of community cards.
/// @param fancy_printing Whether to use fancy printing for the cards.
static void print_player_hand(const Card *hole, const Card *community, int n_community,
      bool fancy_printing) {
	printf("Your hand: ");
	print_cards(hole, 2, fancy_printing);
	if (n_community > 0) {
		Card all[7];
		memcpy(all, hole, 2 * sizeof(Card));
		memcpy(all + 2, community, n_community * sizeof(Card));
		HandValue best = find_best_hand(all, 2 + n_community);
		printf("  [best: %s]", hand_rank_str(best.rank));
	}
	printf("\n");
}

/// @brief Plays a single hand of poker.
/// @param player_chips Pointer to the player's chip count.
/// @param dealer_chips Pointer to the dealer's chip count.
/// @param hand_num The hand number (0-indexed). Even = player posts small blind and acts
///                 first post-flop; odd = dealer posts small blind and acts first
///                 post-flop.
/// @return 0 if hand played to completion, 1 if early win/loss already applied
static int play_hand(int *player_chips, int *dealer_chips, bool fancy_printing,
      int hand_num) {
	if (*player_chips < BIG_BLIND) {
		printf("You don't have enough chips to play (need at least %d).\n", BIG_BLIND);
		return 1;
	}
	if (*dealer_chips < BIG_BLIND) {
		printf("Dealer is out of chips!\n");
		return 1;
	}

	Deck deck;
	init_deck(&deck);
	shuffle_deck(&deck);

	/* Even-numbered hands: player = small blind, player acts first post-flop.
	 * Odd-numbered  hands: dealer = small blind, dealer acts first post-flop. */
	bool player_is_sb = (hand_num % 2 == 0);

	// Post blinds
	int player_blind = player_is_sb ? SMALL_BLIND : BIG_BLIND;
	int dealer_blind = player_is_sb ? BIG_BLIND : SMALL_BLIND;
	if (player_blind > *player_chips) {
		player_blind = *player_chips;
	}
	if (dealer_blind > *dealer_chips) {
		dealer_blind = *dealer_chips;
	}

	*player_chips -= player_blind;
	*dealer_chips -= dealer_blind;
	int pot = player_blind + dealer_blind;

	printf("\n╔══════════════════════════════════════╗\n");
	printf("║               NEW HAND               ║\n");
	printf("╚══════════════════════════════════════╝\n");
	if (player_is_sb) {
		printf("Small blind (you): %d  |  Big blind (dealer): %d\n", player_blind,
		      dealer_blind);
	} else {
		printf("Small blind (dealer): %d  |  Big blind (you): %d\n", dealer_blind,
		      player_blind);
	}
	printf("Pot: %d\n", pot);

	// Deal cards to player and dealer
	Card player_hole[2], dealer_hole[2];
	player_hole[0] = deal_card(&deck);
	dealer_hole[0] = deal_card(&deck);
	player_hole[1] = deal_card(&deck);
	dealer_hole[1] = deal_card(&deck);

	printf("Your hole cards: ");
	print_cards(player_hole, 2, fancy_printing);
	printf("\n");

	/* ── Pre-Flop ── */
	int player_bet = player_blind, dealer_bet = dealer_blind;
	BettingResult result = betting_round("Pre-Flop", player_chips, dealer_chips, &pot,
	      player_bet, dealer_bet, player_is_sb);
	if (result != BETTING_COMPLETE) {
		// someone folded
		return 0;
	}

	bool postflop_player_first = player_is_sb;

	/* ── Flop ── */
	Card community[5];
	community[0] = deal_card(&deck);
	community[1] = deal_card(&deck);
	community[2] = deal_card(&deck);
	printf("\nFlop: ");
	print_cards(community, 3, fancy_printing);
	printf("\n");
	print_player_hand(player_hole, community, 3, fancy_printing);

	result = betting_round("Flop", player_chips, dealer_chips, &pot, 0, 0,
	      postflop_player_first);
	if (result != BETTING_COMPLETE) {
		// someone folded
		return 0;
	}

	/* ── Turn ── */
	community[3] = deal_card(&deck);
	printf("\nTurn: ");
	print_cards(community, 4, fancy_printing);
	printf("\n");
	print_player_hand(player_hole, community, 4, fancy_printing);

	result = betting_round("Turn", player_chips, dealer_chips, &pot, 0, 0,
	      postflop_player_first);
	if (result != BETTING_COMPLETE) {
		// someone folded
		return 0;
	}

	/* ── River ── */
	community[4] = deal_card(&deck);
	printf("\nRiver: ");
	print_cards(community, 5, fancy_printing);
	printf("\n");
	print_player_hand(player_hole, community, 5, fancy_printing);

	result = betting_round("River", player_chips, dealer_chips, &pot, 0, 0,
	      postflop_player_first);
	if (result != BETTING_COMPLETE) {
		// someone folded
		return 0;
	}

	/* ── Showdown ── */
	showdown(player_hole, dealer_hole, community, 5, player_chips, dealer_chips, &pot,
	      fancy_printing);
	return 0;
}

static void print_intro(int *player_chips, int *dealer_chips) {
	printf("Enter your name: ");
	fgets(player_name, sizeof(player_name), stdin);
	player_name[strcspn(player_name, "\r\n")] = '\0';

	printf("╔══════════════════════════════════════════╗\n");
	printf("║          ♣ ♦ Texas Hold'em ♥ ♠           ║\n");
	printf("╠══════════════════════════════════════════╣\n");
	printf("║  Small blind: %-4d  Big blind: %-4d      ║\n", SMALL_BLIND, BIG_BLIND);
	printf("║  Blinds alternate each hand              ║\n");
	printf("║  Starting chips: %-4d each               ║\n", STARTING_CHIPS);
	printf("╚══════════════════════════════════════════╝\n");
	printf("Welcome to the table, ");
	printf(player_name);
	printf("!\n");
	printf("Actions: check  |  raise <n>  |  call  |  fold\n");
	printf("Raises must be in increments of the big blind (%d).\n\n", BIG_BLIND);
	(void) player_chips;
	(void) dealer_chips;
}

int main(void) {
	srand((unsigned) time(NULL));

	setvbuf(stdout, NULL, _IONBF, 0);

	int player_chips = STARTING_CHIPS;
	int dealer_chips = STARTING_CHIPS;

	print_intro(&player_chips, &dealer_chips);

	bool fancy_printing = true;
	int hand_num = 0;

	char buf[16];
	while (player_chips > 0 && dealer_chips > 0) {
		printf("\n──────────────────────────────────────────\n");
		printf("Your chips: %d  |  Dealer chips: %d\n", player_chips, dealer_chips);
		printf("Play a hand? (y to play / n to exit / t to toggle Unicode suits "
		       "[currently %s]): ",
		      fancy_printing ? "on" : "off");
		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			break;
		}
		if (buf[0] == 't' || buf[0] == 'T') {
			fancy_printing = !fancy_printing;
			printf("Unicode suits %s.\n", fancy_printing ? "enabled" : "disabled");
			continue;
		}
		if (buf[0] != 'y' && buf[0] != 'Y') {
			printf("Thanks for playing, %s! Final chips: %d\n", player_name,
			      player_chips);
			break;
		}

		play_hand(&player_chips, &dealer_chips, fancy_printing, hand_num);
		hand_num++;

		if (player_chips <= 0) {
			printf("\nYou're out of chips. Game over!\n");
		} else if (dealer_chips <= 0) {
			printf("\nThe dealer is out of chips. You win the game!\n");
		}
	}
	if (player_chips > 2 * STARTING_CHIPS) {
		FILE *f = fopen("flag.txt", "r");
		if (!f) {
			printf("Congratulations on winning, but the flag file is missing. Contact "
			       "the administrator.\n");
			return 1;
		}
		char flag[128];
		if (fgets(flag, sizeof(flag), f) == NULL) {
			printf("Congratulations on winning, but the flag file is empty. Contact the "
			       "administrator.\n");
			fclose(f);
			return 1;
		}
		fclose(f);
		printf("\nIncredible! You've minted new chips and won the prize: %s\n",
		      flag);
	} else {
		printf("\nYou finished with %d chips. If you want a prize, better luck next time!\n", player_chips);
	}

	return 0;
}
