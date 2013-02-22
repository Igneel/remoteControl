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
int H[WorldSize][WorldSize]={0};  //  стоимость
int h_evr[WorldSize][WorldSize]={0}; // эвристика
int map[WorldSize][WorldSize]={0};   // карта, с металлическими объектами

void SendData(char * data);
void GetData(char *buffer);
bool started=0;

int maxX=0;
int maxY=0;












/*
При установке связи система выделяет два ком-порта для взаимодействия с усройством
Хотя при работе собственно используется только один порт, самый простой способ его найти -
посмотреть в свойствах оба порта и проверить какой из них не открывается при выключенном роботе
*/

HANDLE Port; // Дескриптор порта
			   // имена портов на чтение и запись
TCHAR PortName[MAX_PATH] = TEXT("\\\\.\\COM10"); // если что-то не работает, проверить соответствие

HANDLE openPort(TCHAR * data);
HANDLE openPort(TCHAR * data)
{
	HANDLE Port;   //Дескриптор COM-порта
	//Открываем COM-порт
	Port = CreateFile(data, //Имя COM-порта
			   GENERIC_READ|GENERIC_WRITE,
			   FILE_SHARE_READ,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
	if (Port == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, TEXT("Невозможно открыть последовательный порт"), TEXT("Error"), MB_OK);
		return 0;
	}

	//Получаем состояние управляющей структуры COM-порта,
	//если не удалось выводим сообщение об ошибке и выходим из
	//обработчика
	COMMCONFIG comm;
	GetCommState(Port, &comm.dcb);

	//Настраиваем управляющую структуру COM-порта
	comm.dcb.BaudRate=CBR_9600;
	comm.dcb.ByteSize = 8;        //Размер байта
	comm.dcb.Parity = NOPARITY;    //Паритет отключен
	comm.dcb.StopBits = ONESTOPBIT;//Один стоповый бит
	//Можно также воспользоваться стандартным диалоговым
	//окном настройки управляющей структуры COM-порта:
	CommConfigDialog(data,NULL,&comm);

	//Применяем настроенную структуру к COM-порту, если не
	//удалось выводим сообщение об ошибке
	if(!SetCommState(Port, &comm.dcb))
	{
		MessageBox(NULL, TEXT("Невозможно сконфигурировать последовательный порт"), TEXT("Error"), MB_OK);
		CloseHandle(Port);
		return 0;

	}

	//Получаем текущие настройки тайм-аутов COM-порта
	COMMTIMEOUTS commTimeouts;
	GetCommTimeouts(Port, &commTimeouts);

	//Перенастраиваем тайм-ауты:
	//Максимальный интервал чтения в миллисекундах между
	//двумя принимаемыми символами
	commTimeouts.ReadIntervalTimeout = 100; //100
	//Константа в миллисекундах используемая для вычисления
	//полного тайм-аута операции чтения
	commTimeouts.ReadTotalTimeoutConstant = 300; // 300
	//Множитель используемый для вычисления полного тайм-аута
	//операции чтения в миллисекундах
	commTimeouts.ReadTotalTimeoutMultiplier = 50;      //50
	//Полный максимальный тайм-аут операции чтения
	//вычисляется следующим образом //ReadTotalTimeoutConstant + (ReadIntervalTimeout * количество считываемых байт)

	//Устанавливаем тайм-ауты для COM-порта
	if(!SetCommTimeouts(Port,&commTimeouts))
	{
		MessageBox(NULL, TEXT("Невозможно настроить тайм-ауты последовательного порта"), TEXT("Error"), MB_OK);
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
		Memo1->Lines->Add("Порт открыт");
}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::GetButtonClick(TObject *Sender)
{

	if(Port)
	{
		DWORD feedback;
		//Чтение данных можно реализовать следующим образом (лучше это делать по таймеру):
		feedback = 1;
		int i=0;
		char ct2=0;
		char dataRead[256] = {0};
		//Попытка чтения первого символа хранящегося в COM-порте
		ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
		if(feedback>0)
		{
		//Если чтение удалось продолжаем чтение пока не встретится символ #13
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
	LONG lResult=1; //Помещаем сюда количество данных, которые
	int i=0;
	// необходимо передать
	if((!WriteFile(Port, &data[0], lResult*sizeof(data[0]), &feedback, 0) || feedback != lResult*sizeof(data[i])))
	{
		CloseHandle(Port);
		Port = INVALID_HANDLE_VALUE;
	}
	//При передаче мы проверяем количество переданных байт, и вообще удалось ли выполнить передачу.

}
//---------------------------------------------------------------------------

void __fastcall TSDIAppForm::DisconnectButtonClick(TObject *Sender)
{
	//И закрываем порт после работы:
	if(Port != INVALID_HANDLE_VALUE)
	{    //Если порт открыт
		CloseHandle(Port);
	}
}
//---------------------------------------------------------------------------

// для отладки - посылает значения от 00 до 0хFF и читает полученные значения
// после чего сохраняет в файл
// отладочная функция, в релизе её не будет.
void __fastcall TSDIAppForm::Button1Click(TObject *Sender)
{
	TStringList *t=new TStringList();
	unsigned char data=0x00;
	DWORD feedback;
	unsigned char ct2=0;
	LONG lResult=1; //Помещаем сюда количество данных, которые
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


//Чтение данных по таймеру:
/*
Схема пока такая - по таймеру вызывается интерактинг, в нем ежели соединение есть - получаем данные.
Блоком, по 28 символов, в следующий символ накатываем конец, возвращаем все что прочитали.

1.Основная проблема - при подключении мы не всегда начинаем получать данные с самого начала, могут быть обрывы, т.е.
после установления соединения компьютер получает обрывок команды и первую часть следующей команды - в итоге ничего не работает.

Далее полученная команда анализируется, разбивается на части (имя команды, тип, координаты, значение) и производятся необходимые действия.


*/
void GetData(char *dataRead)
{
	if(Port)
	{
		DWORD feedback;
		feedback = 1;
		int i=0;
		char ct2=0;

		//Попытка чтения первого символа хранящегося в COM-порте
		ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
		if(feedback>0)
		{
		//Если чтение удалось продолжаем чтение пока не встретится символ #13
			do
			{
				dataRead[i] = ct2;


				ReadFile(Port, &ct2, sizeof(ct2), &feedback, NULL);
				i++;
			}
			while(feedback>0);
				dataRead[i] = '\0';
				if(i<28) dataRead[0]='\0';
				// пока 28, но возможно нужно будет ставить 29 - проверить при отладке.
		}
	}
}

	String command;//=strtok(dataRead," \n\r"); // распознаем команду
	String data1;//=strtok(NULL," \n\r");       // выделяем первую координату
	String data2;//=strtok(NULL," \n\r");       // вторую координату
	String data3;//=strtok(NULL," \n\r");       // и третий параметр, если есть
//---------------------------------------------------------------------------
void Obrabotka();
int cxx=0,cyy=0;
void Interracting(String data="") // функция взаимодействия с роботом.
{
	String s;

	if(!Port) return; // если нет соединения - пока делать нечего
	char dataRead[256] = {0}; // выделяем память для сообщения
	/*if(data!="")
	{
	for(int i=1;i<data.Length();i++)
	dataRead[i]=data.operator [](i);
	s=data;
	}
	else
	{   */
		GetData(dataRead);   // читаем данные, пока они не кончатся, или не встретим \r
		if(dataRead[0]=='\0') // если пусто - делать пока нечего.
		{
			if(!started)
				SendData("temp");
			return;
		}
		s=dataRead;
    //}

	SDIAppForm->Memo1->Lines->Add("get:"); // для протокола и отладки выводим полученную строку на экран
	SDIAppForm->Memo1->Lines->Add(s);
	command=strtok(dataRead," \n\r"); // распознаем команду

	String dataGS=strtok(NULL," \n\r");
	data1=strtok(NULL," \n\r");       // выделяем первую координату
	data2=strtok(NULL," \n\r");       // вторую координату
	data3=strtok(NULL," \n\r");       // и третий параметр, если есть
	//String next=strtok(NULL,""); // другие команды если есть


	if(command==NULL)
	return;
	if(command=="Hinyt" || command=="test")
	 {
	 char temp='A';
		 SendData(&temp);
     }

	if(dataGS=="s") // если это установка параметра
	{
	int d3=StrToInt(data3);
	char temp='A';
		if(command=="max") // установка значения для карты
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
		if(command=="Hint") // установка значения для карты
		{

			H[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}
		if(command=="hevr") // установка значения для эвристики
		{
			h_evr[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}
		if(command=="Metall") // установка для карты металлических объектов.
		{
			map[StrToInt(data1)][StrToInt(data2)]=d3;
			SendData(&temp);
		}

		// заполнение массива эвристических оценок.
		//---------------------------------------------------------------------
		//---------------------------------------------------------------------
		for(int i=0;i<=maxX;i++) // по координате х
			for(int j=0;j<=maxY;j++) // по координате y
			{
				int temp=maxX+maxY;
				for(int k=0;k<=maxX;k++)  // для каждой клетки
					for (int l=0; l <=maxY; l++)
					{
						if(H[k][l]==0) // если мы её не посещали - это наша цель
						{
							int r=abs(k-i)+abs(l-j); // находим манхэтенское расстояние
							if(temp>r) // если цель ближе предыдущей
								temp=r;// именно её и запишем в эвристику.
						}
					}
				h_evr[i][j]=temp; // расстояние до ближайшей цели.

			}
		//---------------------------------------------------------------------
		//---------------------------------------------------------------------

		SDIAppForm->FormCreate(NULL);
	}
	else // если это получение параметра
	{
		if(command=="start") // начало работы.
		{
			char temp=StrToInt(SDIAppForm->Edit3->Text);
			if(temp==13)
				started=1;
			SendData(&temp);
		}
		if(command=="jbsdne") // окончание работы
		{
			char temp=StrToInt(SDIAppForm->Edit4->Text);
			SendData(&temp);
		}
		if(command=="Hint") // получение значения карты
		{
			char temp=H[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="H+hevr") // получение суммы карты и эвристики
		{               // сильно снижает нагрузку на канал.
			char temp=H[StrToInt(data1)][StrToInt(data2)]+h_evr[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="hevr") // получение значения эвристики.
		{
			char temp=h_evr[StrToInt(data1)][StrToInt(data2)];
			SendData(&temp);
		}
		if(command=="Calc")
		{
		// функция эвристической оценки.
		// нуждается в серьёзной доработке, т.к. на данный момент выводит не очень приятные результаты.
		/*
		сейчас в ней учтено:
		1. отрицательные значения отсеиваются.
		2. текущее положение не выбирается, т.е. робот не имеет возможности оставаться на месте.
		3. выбирается последнее минимальное значение среди всех имеющихся.

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
	LONG lResult=1; //Помещаем сюда количество данных, которые
	int i=0;
	// необходимо передать
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

