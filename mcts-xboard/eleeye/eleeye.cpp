/*
eleeye.cpp - Source Code for ElephantEye, Part IX

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.3, Last Modified: Mar. 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include "../base/base2.h"
#include "../base/parse.h"
#include "ucci.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#include "search.h"
#include <time.h>

const int INTERRUPT_COUNT = 4096; // 搜索若干结点后调用中断

extern Session* session;

inline void PrintLn(const char *sz) {
  printf("%s\n", sz);
  fflush(stdout);
}
int main(void) {
	int i;
	bool bPonderTime;
	UcciCommStruct UcciComm;
	PositionStruct posProbe;

	if (BootLine() != UCCI_COMM_UCCI) {
		return 0;
	}
	LocatePath(Search.szBookFile, "BOOK.DAT");
	bPonderTime = false;
	PreGenInit();
	srand(time(0));
	NewHash(24); // 24=16MB, 25=32MB, 26=64MB, ...

	//printf("phase0\n");
	Status status = NewSession(SessionOptions(), &session);
	if (!status.ok()) {
		std::cout << status.ToString() << "\n";
		return 1;
	}

	GraphDef graph_def;
	status = ReadBinaryProto(Env::Default(), "models/betaelephant.pb", &graph_def);
	if (!status.ok()) {
		std::cout << status.ToString() << "\n";
		return 1;
	}

	status = session->Create(graph_def);
	if (!status.ok()) {
		std::cout << status.ToString() << "\n";
		return 1;
	}
	//printf("phase1\n");
	printf("ver1\n");
	Search.pos.FromFen(cszStartFen);
	Search.pos.nDistance = 0;
	Search.pos.PreEvaluate();
	Search.nBanMoves = 0;
	Search.bQuit = Search.bBatch = Search.bDebug = false;
	Search.bUseHash = Search.bUseBook = Search.bNullMove = Search.bKnowledge = true;
	Search.bIdle = false;
	Search.nCountMask = INTERRUPT_COUNT - 1;
	Search.nRandomMask = 0;
	Search.rc4Random.InitRand();

	//printf("phase2\n");
	// 以下是接收指令和提供对策的循环体
	while (!Search.bQuit) {
		switch (IdleLine(UcciComm, Search.bDebug)) {
		case UCCI_COMM_ISREADY:
			PrintLn("readyok");
			break;
		case UCCI_COMM_STOP:
			PrintLn("nobestmove");
			break;
		case UCCI_COMM_POSITION:
			BuildPos(Search.pos, UcciComm);
			Search.pos.nDistance = 0;
			Search.pos.PreEvaluate();
			Search.nBanMoves = 0;
			break;
		case UCCI_COMM_MOVE:
			Search.pos.MakeMove(UcciComm.move);
			SearchMain(SEARCHNODE);
			break;
		case UCCI_COMM_BANMOVES:
			Search.nBanMoves = UcciComm.nBanMoveNum;
			for (i = 0; i < UcciComm.nBanMoveNum; i++) {
				Search.wmvBanList[i] = COORD_MOVE(UcciComm.lpdwBanMovesCoord[i]);
			}
			break;
		case UCCI_COMM_SETOPTION:
			switch (UcciComm.Option) {
			case UCCI_OPTION_PROMOTION:
				PreEval.bPromotion = UcciComm.bCheck;
				break;
			case UCCI_OPTION_BATCH:
				Search.bBatch = UcciComm.bCheck;
				break;
			case UCCI_OPTION_DEBUG:
				Search.bDebug = UcciComm.bCheck;
				break;
			case UCCI_OPTION_PONDER:
				bPonderTime = UcciComm.bCheck;
				break;
			case UCCI_OPTION_USEHASH:
				Search.bUseHash = UcciComm.bCheck;
				break;
			case UCCI_OPTION_USEBOOK:
				Search.bUseBook = UcciComm.bCheck;
				break;
			case UCCI_OPTION_BOOKFILES:
				if (AbsolutePath(UcciComm.szOption)) {
					strcpy(Search.szBookFile, UcciComm.szOption);
				}
				else {
					LocatePath(Search.szBookFile, UcciComm.szOption);
				}
				break;
			case UCCI_OPTION_HASHSIZE:
				DelHash();
				i = 19; // 小于1，分配0.5M置换表
				while (UcciComm.nSpin > 0) {
					UcciComm.nSpin /= 2;
					i++;
				}
				NewHash(MMAX(i, 24)); // 最小的置换表设为16M
				break;
			case UCCI_OPTION_IDLE:
				switch (UcciComm.Grade) {
				case UCCI_GRADE_NONE:
					Search.bIdle = false;
					Search.nCountMask = INTERRUPT_COUNT - 1;
					break;
				case UCCI_GRADE_SMALL:
					Search.bIdle = true;
					Search.nCountMask = INTERRUPT_COUNT / 4 - 1;
					break;
				case UCCI_GRADE_MEDIUM:
					Search.bIdle = true;
					Search.nCountMask = INTERRUPT_COUNT / 16 - 1;
					break;
				case UCCI_GRADE_LARGE:
					Search.bIdle = true;
					Search.nCountMask = INTERRUPT_COUNT / 64 - 1;
					break;
				default:
					break;
				}
				break;
			case UCCI_OPTION_PRUNING:
				Search.bNullMove = (UcciComm.Grade != UCCI_GRADE_NONE);
				break;
			case UCCI_OPTION_KNOWLEDGE:
				Search.bKnowledge = (UcciComm.Grade != UCCI_GRADE_NONE);
				break;
			case UCCI_OPTION_RANDOMNESS:
				switch (UcciComm.Grade) {
				case UCCI_GRADE_NONE:
					Search.nRandomMask = 0;
					break;
				case UCCI_GRADE_TINY:
					Search.nRandomMask = 1;
					break;
				case UCCI_GRADE_SMALL:
					Search.nRandomMask = 3;
					break;
				case UCCI_GRADE_MEDIUM:
					Search.nRandomMask = 7;
					break;
				case UCCI_GRADE_LARGE:
					Search.nRandomMask = 15;
					break;
				case UCCI_GRADE_HUGE:
					Search.nRandomMask = 31;
					break;
				default:
					break;
				}
				break;
			case UCCI_OPTION_NEWGAME:
				Search.pos.FromFen("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1");
				break;
			default:
				break;
			}
			break;
		case UCCI_COMM_GO:
			Search.bPonder = UcciComm.bPonder;
			Search.bDraw = UcciComm.bDraw;
			switch (UcciComm.Go) {
			case UCCI_GO_DEPTH:
				Search.nGoMode = GO_MODE_INFINITY;
				Search.nNodes = 0;
				SearchMain(SEARCHNODE);
				break;
			case UCCI_GO_NODES:
				Search.nGoMode = GO_MODE_NODES;
				Search.nNodes = UcciComm.nNodes;
				SearchMain(UcciComm.nNodes);
				break;
			case UCCI_GO_TIME_MOVESTOGO:
			case UCCI_GO_TIME_INCREMENT:
				Search.nGoMode = GO_MODE_TIMER;
				if (UcciComm.Go == UCCI_GO_TIME_MOVESTOGO) {
					// 对于时段制，把剩余时间平均分配到每一步，作为适当时限。
					// 剩余步数从1到5，最大时限依次是剩余时间的100%、90%、80%、70%和60%，5以上都是50%
					Search.nProperTimer = UcciComm.nTime / UcciComm.nMovesToGo;
					Search.nMaxTimer = UcciComm.nTime * MMAX(5, 11 - UcciComm.nMovesToGo) / 10;
				}
				else {
					// 对于加时制，假设棋局会在20回合内结束，算出平均每一步的适当时限，最大时限是剩余时间的一半
					Search.nProperTimer = UcciComm.nTime / 20 + UcciComm.nIncrement;
					Search.nMaxTimer = UcciComm.nTime / 2;
				}
				// 如果是后台思考的时间分配策略，那么适当时限设为原来的1.25倍
				Search.nProperTimer += (bPonderTime ? Search.nProperTimer / 4 : 0);
				Search.nMaxTimer = MMIN(Search.nMaxTimer, Search.nProperTimer * 10);
				SearchMain(SEARCHNODE);
				break;
			default:
				SearchMain(SEARCHNODE);
				break;
			}
			break;
		case UCCI_COMM_PROBE:
			BuildPos(posProbe, UcciComm);
			if (!PopHash(posProbe)) {
				PopLeaf(posProbe);
			}
			break;
		case UCCI_COMM_QUIT:
			Search.bQuit = true;
			break;
		case UCCI_COMM_NEWGAME:
			Search.pos.FromFen("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1");
			break;
		default:
			break;
		}
	}
	DelHash();
	PrintLn("bye");
	return 0;
}