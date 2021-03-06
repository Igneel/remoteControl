//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "SDIMain.h"
#include "About.h"
//---------------------------------------------------------------------
#pragma resource "*.dfm"
TSDIAppForm *SDIAppForm;



// --------------------------------------------------------------------------
const int WorldSize=30;
int H[WorldSize][WorldSize]={0};  //  ���������
int h_evr[WorldSize][WorldSize]={0}; // ���������
int map[WorldSize][WorldSize]={0};   // �����, � �������������� ���������

void SendData(char * data);
void GetData(char *buffer);
bool started=0;

int maxX=0;
int maxY=0;












/*
��� ��������� ����� ������� �������� ��� ���-����� ��� �������������� � ����������
���� ��� ������ ���������� ������������ ������ ���� ����, ����� ������� ������ ��� ����� -
���������� � ��������� ��� ����� � ��������� ����� �� ��� �� ����������� ��� ����������� ������
*/

HANDLE Port; // ���������� �����
			   // ����� ������ �� ������ � ������
TCHAR PortName[MAX_PATH] = TEXT("\\\\.\\COM10"); // ���� ���-�� �� ��������, ��������� ������������

HANDLE openPort(TCHAR * data);
HANDLE openPort(TCHAR * data)
{
	HANDLE Port;   //���������� COM-�����
	//��������� COM-����
	Port = CreateFile(data, //��� COM-�����
			   GENERIC_READ|GENERIC_WRITE,
			   FILE_SHARE_READ,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
	if (Port == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, TEXT("���������� ������� ���������������� ����"), TEXT("Error"), MB_OK);
		return 0;
	}

	//�������� ��������� ����������� ��������� COM-�����,
	//���� �� ������� ������� ��������� �� ������ � ������� ��
	//�����������
	COMMCONFIG comm;
	GetCommState(Port, &comm.dcb);

	//����������� ����������� ��������� COM-�����
	comm.dcb.BaudRate=CBR_9600;
	comm.dcb.ByteSize = 8;        //������ �����
	comm.dcb.Parity = NOPARITY;    //������� ��������
	comm.dcb.StopBits = ONESTOPBIT;//���� �������� ���
	//����� ����� ��������������� ����������� ����������
	//����� ��������� ����������� ��������� COM-�����:
	CommConfigDialog(data,NULL,&comm);

	//��������� ����������� ��������� � COM-�����, ���� ��
	//������� ������� ��������� �� ������
	if(!SetCommState(Port, &comm.dcb))
	{
		MessageBox(NULL, TEXT("���������� ���������������� ���������������� ����"), TEXT("Error"), MB_OK);
		CloseHandle(Port);
		return 0;

	}

	//�������� ������� ��������� ����-����� COM-�����
	COMMTIMEOUTS commTimeouts;
	GetCommTimeouts(Port, &commTimeouts);

	//��������������� ����-����:
	//������������ �������� ������ � ������������� �����
	//����� ������������ ���������
	commTimeouts.ReadIntervalTimeout = 100; //100
	//��������� � ������������� ������������ ��� ����������
	//������� ����-���� �������� ������
	commTimeouts.ReadTotalTimeoutConstant = 300; // 300
	//��������� ������������ ��� ���������� ������� ����-����
	//�������� ������ � �������������
	commTimeouts.ReadTotalTimeoutMultiplier = 50;      //50
	//������ ������������ ����-��� �������� ������
	//����������� ��������� ������� //ReadTotalTimeoutConstant + (ReadIntervalTimeout * ���������� ����������� ����)

	//������������� ����-���� ��� COM-�����
	if(!SetCommTimeouts(Port,&commTimeouts))
	{
		MessageBox(NULL, TEXT("���������� ��������� ����-���� ����������������� �����"), TEXT("Error"), MB_OK);
		CloseHandle(Port);
		return 0;
	}

	return Port;
}

//---------------------------------------------------------------------
__fastcall TSDIAppForm::TSDIAppForm(TComponent *AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------

void __fastcall TSDIAppForm::FileNew1Execute(TObject *Sender)
{
  // Do nothing
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::FileOpen1Execute(TObject *Sender)
{
  OpenDialog->Execute();
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::FileSave1Execute(TObject *Sender)
{
  SaveDialog->Execute();
}
//---------------------------------------------------------------------------


void __fastcall TSDIAppForm::FileExit1Execute(TObject *Sender)
{
  Close();
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::HelpAbout1Execute(TObject *Sender)
{
  AboutBox->ShowModal();
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::ConnectButtonClick(TObject *Sender)
{
	Port=openPort(PortName);
	if(Port!=0)
		Memo1->Lines->Add("���� ������");
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::GetButtonClick(TObject *Sender)
{

	if(Port)
	{
		DWORD feedback;
		//������ ������ ����� ����������� ��������� ������� (����� ��� ������ �� �������):
		feedback = 1;
		int i=0;
		char ct2=0;
		char dataRead[256] = {0};
		//������� ������ ������� ������� ����������� � COM-�����
		ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
		if(feedback>0)
		{
		//���� ������ ������� ���������� ������ ���� �� ���������� ������ #13
			do
			{
				dataRead[i] = ct2;// & 111; //127
				ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
				i++;
			}
			while(feedback>0 && i<254);
			dataRead[i] = '\0';
		}
		if(dataRead!="\0")
		{
			Memo1->Lines->Add("data is:");
			Memo1->Lines->Add(dataRead);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::SendButtonClick(TObject *Sender)
{

	char *data=AnsiString(Edit2->Text).c_str();
	data[0]='\r';
	data[1]='\0';
	DWORD feedback;
	LONG lResult=1; //�������� ���� ���������� ������, �������
	int i=0;
	// ���������� ��������
	if((!WriteFile(Port, &data[0], lResult*sizeof(data[0]), &feedback, 0) || feedback != lResult*sizeof(data[i])))
	{
		CloseHandle(Port);
		Port = INVALID_HANDLE_VALUE;
	}
	//��� �������� �� ��������� ���������� ���������� ����, � ������ ������� �� ��������� ��������.

}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::DisconnectButtonClick(TObject *Sender)
{
	//� ��������� ���� ����� ������:
	if(Port != INVALID_HANDLE_VALUE)
	{    //���� ���� ������
		CloseHandle(Port);
	}
}
//---------------------------------------------------------------------------

// ��� ������� - �������� �������� �� 00 �� 0�FF � ������ ���������� ��������
// ����� ���� ��������� � ����
// ���������� �������, � ������ � �� �����.
void __fastcall TSDIAppForm::Button1Click(TObject *Sender)
{
	TStringList *t=new TStringList();
	unsigned char data=0x00;
	DWORD feedback;
	unsigned char ct2=0;
	LONG lResult=1; //�������� ���� ���������� ������, �������
	int i=0;
	for(data=0;data<255;data++)
	{
		WriteFile(Port, &data, lResult*sizeof(data), &feedback, 0) || feedback != lResult*sizeof(data);
		ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
		Memo1->Lines->Add((unsigned int)data);
		Memo1->Lines->Add((unsigned int)ct2);
	}
	if(SaveDialog->Execute())
	{
		t->Text=Memo1->Text;
		t->SaveToFile(SaveDialog->FileName);
	}
}


//������ ������ �� �������:
/*
����� ���� ����� - �� ������� ���������� �����������, � ��� ����� ���������� ���� - �������� ������.
������, �� 28 ��������, � ��������� ������ ���������� �����, ���������� ��� ��� ���������.

1.�������� �������� - ��� ����������� �� �� ������ �������� �������� ������ � ������ ������, ����� ���� ������, �.�.
����� ������������ ���������� ��������� �������� ������� ������� � ������ ����� ��������� ������� - � ����� ������ �� ��������.

����� ���������� ������� �������������, ����������� �� ����� (��� �������, ���, ����������, ��������) � ������������ ����������� ��������.


*/
void GetData(char *dataRead)
{
	if(Port)
	{
		DWORD feedback;
		feedback = 1;
		int i=0;
		char ct2=0;

		//������� ������ ������� ������� ����������� � COM-�����
		ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
		if(feedback>0)
		{
		//���� ������ ������� ���������� ������ ���� �� ���������� ������ #13
			do
			{
				dataRead[i] = ct2;


				ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
				i++;
			}
			while(feedback>0);
				dataRead[i] = '\0';
				if(i<28) dataRead[0]='\0';
				// ���� 28, �� �������� ����� ����� ������� 29 - ��������� ��� �������.
		}
	}
}

	String command;//=strtok(dataRead," \n\r"); // ���������� �������
	String data1;//=strtok(NULL," \n\r");       // �������� ������ ����������
	String data2;//=strtok(NULL," \n\r");       // ������ ����������
	String data3;//=strtok(NULL," \n\r");       // � ������ ��������, ���� ����
//---------------------------------------------------------------------------
void Obrabotka();
int cxx=0,cyy=0;
void Interracting(String data="") // ������� �������������� � �������.
{
	String s;

	if(!Port) return; // ���� ��� ���������� - ���� ������ ������
	char dataRead[256] = {0}; // �������� ������ ��� ���������
	/*if(data!="")
	{
	for(int i=1;i<data.Length();i++)
	dataRead[i]=data.operator [](i);
	s=data;
	}
	else
	{   */
		GetData(dataRead);   // ������ ������, ���� ��� �� ��������, ��� �� �������� \r
		if(dataRead[0]=='\0') // ���� ����� - ������ ���� ������.
		{
			if(!started)
				SendData("temp");
			return;
		}
		s=dataRead;
    //}

	SDIAppForm->Memo1->Lines->Add("get:"); // ��� ��������� � ������� ������� ���������� ������ �� �����
	SDIAppForm->Memo1->Lines->Add(s);
	command=strtok(dataRead," \n\r"); // ���������� �������

	String dataGS=strtok(NULL," \n\r");
	data1=strtok(NULL," \n\r");       // �������� ������ ����������
	data2=strtok(NULL," \n\r");       // ������ ����������
	data3=strtok(NULL," \n\r");       // � ������ ��������, ���� ����
	//String next=strtok(NULL,""); // ������ ������� ���� ����


	if(command==NULL)
	return;
	if(command=="Hinyt" || command=="test")
	 {
	 char temp='A';
		 SendData(&temp);
     }

	if(dataGS=="s") // ���� ��� ��������� ���������
	{
	int d3=StrToInt(data3);
	char temp='A';
		if(command=="max") // ��������� �������� ��� �����
		{
			maxX=StrToInt(data1);
			maxY=StrToInt(data2);
			for(int i=maxX+1;i<WorldSize;i++)
			for(int j=0;j<WorldSize;j++)
			H[i][j]=50;
			for(int i=0;i<WorldSize;i++)
			for(int j=maxY;j<WorldSize;j++)
			H[i][j]=50;
			SendData(&temp);
		}
		if(command=="Hint") // ��������� �������� ��� �����
		{

			H[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}
		if(command=="hevr") // ��������� �������� ��� ���������
		{
			h_evr[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}
		if(command=="Metall") // ��������� ��� ����� ������������� ��������.
		{
			map[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}

		// ���������� ������� ������������� ������.
		//---------------------------------------------------------------------
		//---------------------------------------------------------------------
		for(int i=0;i<=maxX;i++) // �� ���������� �
			for(int j=0;j<=maxY;j++) // �� ���������� y
			{
				int temp=maxX+maxY;
				for(int k=0;k<=maxX;k++)  // ��� ������ ������
					for (int l=0; l <=maxY; l++)
					{
						if(H[k][l]==0) // ���� �� � �� �������� - ��� ���� ����
						{
							int r=abs(k-i)+abs(l-j); // ������� ������������ ����������
							if(temp>r) // ���� ���� ����� ����������
								temp=r;// ������ � � ������� � ���������.
						}
					}
				h_evr[i][j]=temp; // ���������� �� ��������� ����.

			}
		//---------------------------------------------------------------------
		//---------------------------------------------------------------------

		SDIAppForm->FormCreate(NULL);
	}
	else // ���� ��� ��������� ���������
	{
		if(command=="start") // ������ ������.
		{
			char temp=StrToInt(SDIAppForm->Edit3->Text);
			if(temp==13)
				started=1;
			SendData(&temp);
		}
		if(command=="jbsdne") // ��������� ������
		{
			char temp=StrToInt(SDIAppForm->Edit4->Text);
			SendData(&temp);
		}
		if(command=="Hint") // ��������� �������� �����
		{
			char temp=H[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="H+hevr") // ��������� ����� ����� � ���������
		{               // ������ ������� �������� �� �����.
			char temp=H[StrToInt(data1)][StrToInt(data2)]+h_evr[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="hevr") // ��������� �������� ���������.
		{
			char temp=h_evr[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="Calc")
		{
		// ������� ������������� ������.
		// ��������� � ��������� ���������, �.�. �� ������ ������ ������� �� ����� �������� ����������.
		/*
		������ � ��� ������:
		1. ������������� �������� �����������.
		2. ������� ��������� �� ����������, �.�. ����� �� ����� ����������� ���������� �� �����.
		3. ���������� ��������� ����������� �������� ����� ���� ���������.

		*/
		int min,temp;
		int x,y;
		x=StrToInt(data1);
		y=StrToInt(data2);
		min=H[x][y+1]+h_evr[x][y+1];//getParam("H+hevr",cX,cY+1);
		cxx=0;
        cyy=1;
		for(int i=-1;i<=1;i++) //
        {
           for(int j=-1;j<=1;j++)//----------------------------------------------------------------------------------------------------------------------!!!!!
			  { //
				if(y+j<0 || x+i<0) continue;
				  if(i==0 && j==0) continue;
				  temp=H[x+i][y+j]+h_evr[x+i][y+j];//getParam("H+hevr",cX+i,cY+j);
				  if(temp<=min) //
				  {            //
					 min=temp;
                     cxx=i;
                     cyy=j;
                  }
              }
		}
		char temp1=cxx;
		SendData(&temp1);
		}
		if(command=="Calc2")
		{
			char temp1=cyy;
			SendData(&temp1);
        }
	}

	}

void SendData(char * data)
{
	SDIAppForm->Memo1->Lines->Add("send");
	SDIAppForm->Memo1->Lines->Add(data);
	SDIAppForm->Memo1->Lines->Add(IntToStr(*data));
	DWORD feedback;
	LONG lResult=1; //�������� ���� ���������� ������, �������
	int i=0;
	// ���������� ��������
	if((!WriteFile(Port, &data[0], lResult*sizeof(data[0]), &feedback, 0) || feedback != lResult*sizeof(data[i])))
	{
		CloseHandle(Port);
		Port = INVALID_HANDLE_VALUE;
	}
}


void __fastcall TSDIAppForm::Button2Click(TObject *Sender)
{
	Interracting();
}
//---------------------------------------------------------------------------
void __fastcall TSDIAppForm::FormCreate(TObject *Sender)
{
	for(int i=0;i<StringGrid1->ColCount;i++)
		for(int j=0;j<StringGrid1->ColCount;j++)
			StringGrid1->Cells[j][i]=map[i][j];
	for(int i=0;i<StringGrid2->ColCount;i++)
		for(int j=0;j<StringGrid2->ColCount;j++)
			StringGrid2->Cells[j][i]=H[i][j]+h_evr[i][j];
}
//---------------------------------------------------------------------------
void Obrabotka()
{

}

