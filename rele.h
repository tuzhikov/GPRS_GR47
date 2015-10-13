#ifndef RELE_H
#define RELE_H


// �������������� ����������:
#define N_RELE      4 // ���-�� ���.����-�
#define P_r1     P2_0  //P3_4
#define P_r2     P2_1  //P3_5
#define P_r3     P2_2  //P3_6
#define P_r4     P2_3  //P3_7

#define port_rele       P2      //P3
#define port_rele_mask  0x0F    //0xF0

// ����� ����� ������� ��������� ����
extern unsigned long xdata sTime_rele_on[N_RELE];
// ����� �� ������� ��������� ����
extern unsigned long xdata sTime_rele_work[N_RELE];

// ����� � ������� ����������
// ����� ����� ������� ��������� ����
extern unsigned long xdata time_rele_on[N_RELE];
// ����� �� ������� ��������� ����
extern unsigned long xdata time_rele_work[N_RELE];

// ����/���������� ��� ����� ������� � ����� �� ���� 'N' - ������; '1' - ��������; '0' - ���������;
extern unsigned char xdata _c_rele_do[N_RELE];

extern unsigned char c_Rele; // ����� ����� ��������� ���������� � ���/���� ����

void initialize_rele(void);
unsigned char get_sost_rele(void);
void automat_rele(void);
void UprRele(unsigned char *buf);

#endif
