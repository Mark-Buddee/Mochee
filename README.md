
# **Mochee** ( MO-chee ) Mother of Chess Engines

Mochee is a UCI chess engine written just for fun in C. Mochee relies heavily on an efficient search routine, evaluating positions only on material count and static piece-square-tables. Designed and written by Buddee890, with inspiration from MisterQueen, and Stockfish.

## Released to Lichess
Mochee was released as a bot on lichess, go and check it out: https://lichess.org/@/MocheeZero

## Goals
Mochee was initially developed to crush the following friends of mine decisively, in every match:
- nickbrowne
- KDAWWWWWGGGGGSSS
- hame_33
- xsing20
- dsingarayar
- Hogbellyman
- bsingarayar
- Adstar_1
- jagabee231

With this achievement on the horizon, Mochee's new goal is to reach 2500 rapid rating on Lichess, with entirely self-trained values. That would confidently place Mochee in the top 0.01% of human players on the site.

## Architecture
Mochee is a single-threaded, classic alpha-beta chess engine relying on well documented search- , evaluation- , and time management techniques. Eventually Mochee will use an Efficiently Updatable Neural Network (NNUE) for static position evaluation using entirely self-trained values.

In order of implementation, Mochee employs (or plans to employ) the following techniques:

### Search
- [x] Minimax
- [x] Alpha-beta pruning
- [x] Transposition table
- [x] Iterative deepening
- [x] Quiescent search extension
- [x] Delta pruning
- [x] Move ordering (hand-crafted)
- [ ] Aspiration windows
- [ ] Quiescent-search transposition table
- [ ] Killer moves
- [ ] Null move pruning
- [ ] Futility pruning / razoring
- [ ] Pondering
- [ ] Multithreaded

### Evaluation
- [x] Material count
- [x] Piece-square tables
- [ ] Endgame Table Base (EGTB)
- [ ] Efficiently Updatable Neural Network (NNUE)

### Time Management
- [x] Move-time classification by game type (Rapid/Blitz/Bullet) and time pressure (None/Standard/High)
- [x] Move making at end of search iteration
- [ ] Move making at sufficient search stability using conspiracy numbers

