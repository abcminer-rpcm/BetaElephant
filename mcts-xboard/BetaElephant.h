#include <vector>
#include "eleeye/position.h"
using namespace std;

void psigma(const char *const szFen, const int N, const int *const moves, double *const ps);
//szFen�Ǿ����Fen����N���ŷ�������move���ŷ������飨����ΪN���ŷ����飩�������ǲ����޸ĵ�����
//p�ǳ���ΪN�ĸ������飬���Ӧ�ŷ������˳�����ÿ���ŷ���psigma

int ppi(const char *const szFen, const int N, const int *const moves);
//�������ƣ�����P�����ŷ����±�

double value(const char *const szFen);
//�������Fen�������غ췽�ľ�������v

int genMove(PositionStruct *pos, int* mv);

char* pos2fen(PositionStruct *pos);

char* genMove(char* szFen);

char* genProtMove(char* szFen);

class Node{
public:
	int K; //legal move count
	bool fullyExpanded;
	PositionStruct pos;
	int move[MAX_GEN_MOVES];
	double P[MAX_GEN_MOVES]; //prior probability
	double Q[MAX_GEN_MOVES]; //action value
	unsigned int N[MAX_GEN_MOVES]; //visit count
	Node* C[MAX_GEN_MOVES]; //children

	int findMaxA(); //�ҵ���ȫ��չNode�����ź���
	int findNew(); //�ҵ�δ��ȫ��չNode����һ����չ�ĺ���
	Node* genNode(int x); //������չ��Node����ʼ��
	double playRollout(); //��ppiģ�⵽��󷵻غ췽�Ƿ��ʤ������0Ϊ����1Ϊʤ
};
