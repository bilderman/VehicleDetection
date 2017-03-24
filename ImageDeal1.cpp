// ImageDeal1.cpp: implementation of the CImageDeal1 class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ImageDeal1.h"
#include "AVIProducer.h"
#include "FlowCounter.h"
//#include "KirschEdgeDetect.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImageDeal1::CImageDeal1()
{
	m_lpStoreBac1=new BYTE[150*320*20*3];//用存放用于背景更新的连续
	m_lpStoreBac2=new BYTE[150*320*20*3];//11帧背景，更新时用第6帧背景
	
	m_lpBacFrame1=new BYTE[320*20*3];//用存放用于背景更新的连续11帧背景，更新时用第6帧背景
	m_lpBacFrame2=new BYTE[320*20*3];
	
	TemStoreBac1=m_lpStoreBac1;
	TemStoreBac2=m_lpStoreBac2;
	
	Initial();
}

CImageDeal1::~CImageDeal1()
{ 
	delete[] m_lpStoreBac1;//用存放用于背景更新的连续
	m_lpStoreBac1=NULL;
	delete[] m_lpStoreBac2;
	m_lpStoreBac2=NULL;	
	KillClass();
}

void CImageDeal1::Initial()
{
	m_pLineSignal=new CLineSignal();
	m_pFlowParaStruc=new TRANSPORTATIONFLOWPARAMETER[1];
	m_pFlowParaStruc->VehicleNumber2=0;
	m_pFlowParaStruc->MinuteVehicleNum2=0;
	m_pFlowParaStruc->Speed2=0;
	m_pFlowParaStruc->TimeDensity2=0;

		
	
	bIsStoreBac=TRUE;
	bIsNewCurFrame=TRUE;

	TempMemory1=0;
	TempMemory2=0;
	NoVehicle1=0;
	NoVehicle2=0;
	NoVehicleFramePos1=0;
	NoVehicleFramePos2=0;
	UpdateNum1=0;//用于调试
	UpdateNum2=0;

	m_lpCurFrame1=NULL;//用于存储当前帧
	m_lpCurFrame2=NULL;

	m_pSubResult1=NULL;//public parameters
	m_pSubResult2=NULL;
	m_pFalseBackground1=NULL;
	m_pFalseBackground2=NULL;
	edge1=NULL;
	edge2=NULL;

}

void CImageDeal1::KillClass()
{	
	
	if(m_lpBacFrame1)
	{
		delete[] m_lpBacFrame1;
		m_lpBacFrame1=NULL;		
	}
	if(m_lpBacFrame2)
	{
		delete[] m_lpBacFrame2;
		m_lpBacFrame2=NULL;		
	}
	m_pLineSignal=NULL;
	delete[] m_pFlowParaStruc;
	m_pFlowParaStruc=NULL;
}

TRANSPORTATIONFLOWPARAMETER* CImageDeal1::GetNextFrame(CDC *pDC,int nLeftPos3,int nRightPos3,int nHeightPos3,int m_nRoadWay11Width,
							  int nLeftPos4,int nRightPos4,int nHeightPos4,int m_nRoadWay12Width,
							  float m_fSignal1,float m_fSignal2,float m_fSpace2,
							  int m_nMaxErrorToler21,int m_nMiniReliability21,int m_nMaxErrorToler22,int m_nMiniReliability22,
							  int nLineHeight,BOOL bIsPause,int nFlag,BOOL bIsShow,BOOL bIsDisplay,LPBYTE m_pSaveImageRegion,DWORD* m_pdwPos,BOOL bIsShowSignal2)
{
	m_nLeftPos3=nLeftPos3;
	m_nRightPos3=nRightPos3;
	m_nHeightPos3=nHeightPos3;
	m_nLeftPos4=nLeftPos4;
	m_nRightPos4=nRightPos4;
	m_nHeightPos4=nHeightPos4;
	m_nLineHeight=nLineHeight;
	m_nLine1Width=abs(m_nRightPos3-m_nLeftPos3);
	m_nLine2Width=abs(m_nRightPos4-m_nLeftPos4);
	TempMemory1=m_nLineHeight*m_nLine1Width*3;
	TempMemory2=m_nLineHeight*m_nLine2Width*3;
//显示停止视频
	if(bIsPause)
		DisplayCurFrame(pDC,nFlag,bIsShow,bIsPause,m_pSaveImageRegion);

//显示连续视频
	if(bIsDisplay)
		DisplayContinuousVideo(pDC,m_nRoadWay11Width,m_nRoadWay12Width,nFlag,bIsShow,bIsPause,m_pSaveImageRegion);
	if(bIsShow)
	{
		SetDetectLine1(pDC,1,m_nHeightPos3,m_nLeftPos3,m_nRightPos3,m_nLineHeight,bIsShow);
		SetDetectLine2(pDC,2,m_nHeightPos4,m_nLeftPos4,m_nRightPos4,m_nLineHeight,bIsShow);
	}
	if((*m_pdwPos)==1)
	{
		UpdateNum2=0;
		UpdateNum1=0;
	}
	CString str;	
	if(bIsDisplay&&bIsShow&&bIsShowSignal2)
	{
		str.Format("上检测线背景更新%d次",UpdateNum2);
		pDC->TextOut(380,185,str);
		str.Format("下检测线背景更新%d次",UpdateNum1);
		pDC->TextOut(380,205,str);
		pDC->TextOut(180,310,"车道2虚拟检测线上的检测信号",27);//坐标轴名称
	}
	if(bIsDisplay)
		m_pFlowParaStruc=DrawLineDealRes(pDC,m_nLeftPos3,m_nRightPos3,m_nHeightPos3,m_nRoadWay11Width,m_nLeftPos4,m_nRightPos4,m_nHeightPos4,m_nRoadWay12Width,
		                                 m_fSignal1,m_fSignal2,m_fSpace2,
		                                 m_nMaxErrorToler21,m_nMiniReliability21,m_nMaxErrorToler22,m_nMiniReliability22,
		                                 m_nLineHeight,bIsShow,m_pdwPos,bIsShowSignal2);
	return m_pFlowParaStruc;
}

void CImageDeal1::DisplayCurFrame(CDC *pDC,int nFlag,BOOL bIsShow,BOOL bIsPause,LPBYTE m_pSaveImageRegion)
{
	LPBYTE lpDetectLine1;
	LPBYTE lpDetectLine2;
	LPBYTE lpBacFrame1Tem;
	LPBYTE lpBacFrame2Tem;

	lpBacFrame1Tem=m_lpBacFrame1;
	lpBacFrame2Tem=m_lpBacFrame2;
	
	lpDetectLine1=m_pSaveImageRegion+320*m_nHeightPos3*3+m_nLeftPos3*3;
	for(int i=0;i<m_nLineHeight;i++)
	{
		memcpy(lpBacFrame1Tem,lpDetectLine1,m_nLine1Width*3);
		lpDetectLine1=lpDetectLine1+320*3;
		lpBacFrame1Tem=lpBacFrame1Tem+m_nLine1Width*3;
	}
			
	lpDetectLine2=m_pSaveImageRegion+320*m_nHeightPos4*3+m_nLeftPos4*3;
	for(int i=0;i<m_nLineHeight;i++)
	{
		memcpy(lpBacFrame2Tem,lpDetectLine2,m_nLine2Width*3);
		lpDetectLine2=lpDetectLine2+320*3;
		lpBacFrame2Tem=lpBacFrame2Tem+m_nLine2Width*3;
	}
		
	lpDetectLine1=NULL;
	lpDetectLine2=NULL;
	lpBacFrame1Tem=NULL;
	lpBacFrame2Tem=NULL;
}

void CImageDeal1::DisplayContinuousVideo(CDC *pDC,int m_nRoadWay11Width,int m_nRoadWay12Width,int nFlag,BOOL bIsShow,BOOL bIsPause,LPBYTE m_pSaveImageRegion)
{
	LPBYTE lpDetectLine1;
	LPBYTE lpDetectLine2;
	LPBYTE lpCurFrame1Tem;
	LPBYTE lpCurFrame2Tem;

	if(bIsNewCurFrame)
	{
		m_lpCurFrame1=new BYTE[m_nLine1Width*m_nLineHeight*3];
		m_lpCurFrame2=new BYTE[m_nLine2Width*m_nLineHeight*3];
		bIsNewCurFrame=FALSE;
	}
		
	lpCurFrame1Tem=m_lpCurFrame1;
	lpCurFrame2Tem=m_lpCurFrame2;

	lpDetectLine1=m_pSaveImageRegion+320*m_nHeightPos3*3+m_nLeftPos3*3;
	for(int i=0;i<m_nLineHeight;i++)
	{
		memcpy(lpCurFrame1Tem,lpDetectLine1,m_nLine1Width*3);
		lpDetectLine1=lpDetectLine1+320*3;
		lpCurFrame1Tem=lpCurFrame1Tem+m_nLine1Width*3;
	}	
		
	lpDetectLine2=m_pSaveImageRegion+320*m_nHeightPos4*3+m_nLeftPos4*3;
	for(int i=0;i<m_nLineHeight;i++)
	{
		memcpy(lpCurFrame2Tem,lpDetectLine2,m_nLine2Width*3);
		lpDetectLine2=lpDetectLine2+320*3;
		lpCurFrame2Tem=lpCurFrame2Tem+m_nLine2Width*3;
	}
//用于显示当前的检测线上的图像
	m_pDib1=new CDib(CSize(m_nLine1Width,m_nLineHeight),24);
	m_pDib2=new CDib(CSize(m_nLine2Width,m_nLineHeight),24);
	if(bIsShow)
	{	
		m_pDib2->m_lpImage=m_lpCurFrame2;
		m_pDib2->Draw(pDC,CPoint(555+m_nRoadWay12Width,129),CSize(m_nLine2Width,m_nLineHeight),bIsShow);
		
		m_pDib1->m_lpImage=m_lpCurFrame1;
		m_pDib1->Draw(pDC,CPoint(555+m_nRoadWay11Width,177),CSize(m_nLine1Width,m_nLineHeight),bIsShow);
	}
	
	LineDeal(pDC,bIsPause);

	UpdateBackground(pDC,m_nRoadWay11Width,m_nRoadWay12Width,bIsPause,bIsShow);	
	
	lpDetectLine1=NULL;
	lpDetectLine2=NULL;	
}

void CImageDeal1::UpdateBackground(CDC* pDC,int m_nRoadWay11Width,int m_nRoadWay12Width,BOOL bIsPause,BOOL bIsShow)
{	
	//检测线1的代码
	if((m_pLineSignal->m_nUnitaryValue1==0)&&(m_pLineSignal->m_nFramePosition==(NoVehicleFramePos1+1)))
	{
		NoVehicle1++;
	//计入内存;
		memcpy(TemStoreBac1,m_lpCurFrame1,TempMemory1);
		TemStoreBac1+=TempMemory1;		
	}
	else
	{
		TemStoreBac1=m_lpStoreBac1;	
		NoVehicle1=0;
	//使内存的值全为0;
	}
	if(NoVehicle1==149)
	{
	//把当前的位置向后减6帧开始的存储的背景作为当前背景，可能使用卡尔曼滤波,然后再清临时内存
		TemStoreBac1-=120*TempMemory1;
		memcpy(m_lpBacFrame1,TemStoreBac1,TempMemory1);
		
		TemStoreBac1+=30*TempMemory1;
		for(int i=0;i<TempMemory1;i++)
		{	
			(*m_lpBacFrame1)=(*m_lpBacFrame1)/4+(*TemStoreBac1)/4;
			m_lpBacFrame1++;
			TemStoreBac1++;
		}
		m_lpBacFrame1-=TempMemory1;

		TemStoreBac1+=29*TempMemory1;
		for(int i=0;i<TempMemory1;i++)
		{	
			(*m_lpBacFrame1)+=(*TemStoreBac1)/4;
			m_lpBacFrame1++;
			TemStoreBac1++;
		}
		m_lpBacFrame1-=TempMemory1;

		TemStoreBac1+=29*TempMemory1;
		for(int i=0;i<TempMemory1;i++)
		{	
			(*m_lpBacFrame1)+=(*TemStoreBac1)/4;
			m_lpBacFrame1++;
			TemStoreBac1++;
		}
		m_lpBacFrame1-=TempMemory1;

		TemStoreBac1=m_lpStoreBac1;	
		NoVehicle1=0;
		UpdateNum1++;//用于调试		
	}
	NoVehicleFramePos1=m_pLineSignal->m_nFramePosition;//为下一次判断用

//检测线2更新
	if((m_pLineSignal->m_nUnitaryValue2<10)&&(m_pLineSignal->m_nFramePosition==(NoVehicleFramePos2+1)))
	{
		NoVehicle2++;
	//计入内存;
		memcpy(TemStoreBac2,m_lpCurFrame2,TempMemory2);
		TemStoreBac2+=TempMemory2;		
	}
	else
	{
		TemStoreBac2=m_lpStoreBac2;
		NoVehicle2=0;
	//使内存的值全为0;
	}
	if(NoVehicle2==149)
	{
	//把当前的位置向后减6帧开始的存储的背景作为当前背景，可能使用卡尔曼滤波,然后再清临时内存
		TemStoreBac2-=120*TempMemory2;
		memcpy(m_lpBacFrame2,TemStoreBac2,TempMemory2);
		
		TemStoreBac2+=30*TempMemory2;
		for(int i=0;i<TempMemory2;i++)
		{	
			(*m_lpBacFrame2)=(*m_lpBacFrame2)/4+(*TemStoreBac2)/4;
			m_lpBacFrame2++;
			TemStoreBac2++;
		}
		m_lpBacFrame2-=TempMemory2;

		TemStoreBac2+=29*TempMemory2;
		for(int i=0;i<TempMemory2;i++)
		{	
			(*m_lpBacFrame2)+=(*TemStoreBac2)/4;
			m_lpBacFrame2++;
			TemStoreBac2++;
		}
		m_lpBacFrame2-=TempMemory2;

		TemStoreBac2+=29*TempMemory2;
		for(int i=0;i<TempMemory2;i++)
		{	
			(*m_lpBacFrame2)+=(*TemStoreBac2)/4;
			m_lpBacFrame2++;
			TemStoreBac2++;
		}
		m_lpBacFrame2-=TempMemory2;
		TemStoreBac2=m_lpStoreBac2;
		NoVehicle2=0;
		UpdateNum2++;//用于调试	
	}
	NoVehicleFramePos2=m_pLineSignal->m_nFramePosition;//为下一次判断用

	m_pDib1=new CDib(CSize(m_nLine1Width,m_nLineHeight),24);
	m_pDib2=new CDib(CSize(m_nLine2Width,m_nLineHeight),24);
	//显示当前的背景//
	if(bIsShow)
	{	
		m_pDib2->m_lpImage=m_lpBacFrame2;
		m_pDib2->Draw(pDC,CPoint(555+m_nRoadWay12Width,25),CSize(m_nLine2Width,m_nLineHeight),bIsShow);

		m_pDib1->m_lpImage=m_lpBacFrame1;
		m_pDib1->Draw(pDC,CPoint(555+m_nRoadWay11Width,73),CSize(m_nLine1Width,m_nLineHeight),bIsShow);
		
	}	
		
	m_pDib1->Empty();
	delete m_pDib1;	
	m_pDib2->Empty();	
	delete m_pDib2;
}

//实现了背景差，其中阈值Threshold=15;设置相应的区域为
void CImageDeal1::LineDeal(CDC *pDC,BOOL bIsPause)
{
	LPBYTE lpCur1=m_lpCurFrame1;
	LPBYTE lpCur2=m_lpCurFrame2;
	LPBYTE lpBac1=m_lpBacFrame1;
	LPBYTE lpBac2=m_lpBacFrame2;
	int r,g,b;
	//int i,j;
	int Threshold=15;
	if(bIsPause)//防止没有连续播放视频时，下一帧为无数据
	{
		lpCur1=m_lpBacFrame1;
		lpCur2=m_lpBacFrame2;
	}
		
	m_pSubResult1=(LPBYTE)new BYTE[m_nLine1Width*m_nLineHeight*3];
	m_pSubResult2=(LPBYTE)new BYTE[m_nLine2Width*m_nLineHeight*3];
	edge1=(LPBYTE)new BYTE[m_nLine1Width*m_nLineHeight*3];
	edge2=(LPBYTE)new BYTE[m_nLine2Width*m_nLineHeight*3];

	m_pFalseBackground1=(LPBYTE)new BYTE[m_nLine1Width*m_nLineHeight*3];
	m_pFalseBackground2=(LPBYTE)new BYTE[m_nLine2Width*m_nLineHeight*3];
//完成背景差和设置伪背景
	//检测线1
	for(int i=0;i<m_nLineHeight;i++)
	{
		for(int j=0;j<m_nLine1Width;j++)
		{	//blue component		
			b=abs(lpCur1[i*m_nLine1Width*3+j*3]-lpBac1[i*m_nLine1Width*3+j*3]);
			if(b<Threshold)
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3]=0;
				m_pFalseBackground1[i*m_nLine1Width*3+j*3]=0;
			}
			else
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3]=lpCur1[i*m_nLine1Width*3+j*3];
				m_pFalseBackground1[i*m_nLine1Width*3+j*3]=lpCur1[i*m_nLine1Width*3+j*3];
			}
			//green component
			g=abs(lpCur1[i*m_nLine1Width*3+j*3+1]-lpBac1[i*m_nLine1Width*3+j*3+1]);
			if(g<Threshold)
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3+1]=0;
				m_pFalseBackground1[i*m_nLine1Width*3+j*3+1]=0;
			}
			else
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3+1]=lpCur1[i*m_nLine1Width*3+j*3+1];
				m_pFalseBackground1[i*m_nLine1Width*3+j*3+1]=lpCur1[i*m_nLine1Width*3+j*3+1];
			}
			//red component
			r=abs(lpCur1[i*m_nLine1Width*3+j*3+2]-lpBac1[i*m_nLine1Width*3+j*3+2]);		
			if(r<Threshold)//没有目标，要实现背景更新
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3+2]=0;
				m_pFalseBackground1[i*m_nLine1Width*3+j*3+2]=0;
			}
			else
			{
				m_pSubResult1[i*m_nLine1Width*3+j*3+2]=lpCur1[i*m_nLine1Width*3+j*3+2];
				m_pFalseBackground1[i*m_nLine1Width*3+j*3+2]=lpCur1[i*m_nLine1Width*3+j*3+2];
			}
		}
		//检测线2
		for(int j=0;j<m_nLine2Width;j++)
		{	//blue component		
			b=abs(lpCur2[i*m_nLine2Width*3+j*3]-lpBac2[i*m_nLine2Width*3+j*3]);
			if(b<Threshold)
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3]=0;
				m_pFalseBackground2[i*m_nLine2Width*3+j*3]=0;
			}
			else
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3]=lpCur2[i*m_nLine2Width*3+j*3];
				m_pFalseBackground2[i*m_nLine2Width*3+j*3]=lpBac2[i*m_nLine2Width*3+j*3];
			}
			//greed component
			g=abs(lpCur2[i*m_nLine2Width*3+j*3+1]-lpBac2[i*m_nLine2Width*3+j*3+1]);
			if(g<Threshold)
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3+1]=0;
				m_pFalseBackground2[i*m_nLine2Width*3+j*3+1]=0;
			}
			else
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3+1]=lpCur2[i*m_nLine2Width*3+j*3+1];
				m_pFalseBackground2[i*m_nLine2Width*3+j*3+1]=lpBac2[i*m_nLine2Width*3+j*3+1];
			}
			//red component
			r=abs(lpCur2[i*m_nLine2Width*3+j*3+2]-lpBac2[i*m_nLine2Width*3+j*3+2]);			
			if(r<Threshold)
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3+2]=0;
				m_pFalseBackground2[i*m_nLine2Width*3+j*3+2]=0;
			}
			else
			{
				m_pSubResult2[i*m_nLine2Width*3+j*3+2]=lpCur2[i*m_nLine2Width*3+j*3+2];
				m_pFalseBackground2[i*m_nLine2Width*3+j*3+2]=lpBac2[i*m_nLine2Width*3+j*3+2];
			}
		}
	}
}
//显示处理的结果：背景差结果、伪背景的结果、调用了
//边缘检测函数kir.KirschEdgeDet()和信号统计函数m_pLineSignal->StatLineSignal()
//TRANSPORTATIONFLOWPARAMETER是在宏定义中定义的交通流参数结构
TRANSPORTATIONFLOWPARAMETER* CImageDeal1::DrawLineDealRes(CDC *pDC,int m_nLeftPos3,int m_nRightPos3,int m_nHeightPos3,int m_nRoadWay11Width,
																	int m_nLeftPos4,int m_nRightPos4,int m_nHeightPos4,int m_nRoadWay12Width,
																	float m_fSignal1,float m_fSignal2,float m_fSpace2,
																	int m_nMaxErrorToler21,int m_nMiniReliability21,int m_nMaxErrorToler22,int m_nMiniReliability22,
																	int m_nLineHeight,BOOL bIsShow,DWORD* m_pdwPos,BOOL bIsShowSignal2)
{
	m_nLine1Width=m_nRightPos3-m_nLeftPos3;
	m_nLine2Width=m_nRightPos4-m_nLeftPos4;
	m_pDib1=new CDib(CSize(m_nLine1Width,m_nLineHeight),24);
	m_pDib2=new CDib(CSize(m_nLine2Width,m_nLineHeight),24);

	LPBYTE tem3=m_pSubResult1;
	LPBYTE tem4=m_pSubResult2;	
    CKirschEdgeDetect kir;
	
	m_pDib1->m_lpImage=m_pSubResult1;
	m_pDib2->m_lpImage=m_pSubResult2;
	//目标分割结果
	if(bIsShow==TRUE)
	{
		//显示试验检测线2区视频
		m_pDib2->Draw(pDC,CPoint(555+m_nRoadWay12Width,233),CSize(m_nLine2Width,m_nLineHeight),bIsShow);
		//显示试验检测线1区视频
		m_pDib1->Draw( pDC,CPoint(555+m_nRoadWay11Width,281),CSize(m_nLine1Width,m_nLineHeight),bIsShow);		
	}

	m_pSubResult1=kir.KirschEdgeDet(pDC,m_nLeftPos3,m_nRightPos3,m_nLineHeight,m_nRoadWay11Width,m_pSubResult1,1,bIsShow,2);
	m_pSubResult2=kir.KirschEdgeDet(pDC,m_nLeftPos4,m_nRightPos4,m_nLineHeight,m_nRoadWay12Width,m_pSubResult2,2,bIsShow,2);
	delete[] tem3;//注意在调用边缘检测时，m_pSubResult1和m_pSubResult1指定的区域发生了变化
	tem3=NULL;//所以才设置中间的tem3和tem4删除原来的内存！
	delete[] tem4;
	tem4=NULL;
	//已经腐蚀过了
//进行轮廓的提出
	//对两块区域进行赋初值
	for(int i=0;i<m_nLineHeight;i++)
		{
			for(int j=0;j<m_nLine1Width;j++)
			{
				edge1[i*m_nLine1Width*3+j*3+0]=0;
				edge1[i*m_nLine1Width*3+j*3+1]=0;
				edge1[i*m_nLine1Width*3+j*3+2]=0;
			}
			for(int j=0;j<m_nLine2Width;j++)
			{
				edge2[i*m_nLine2Width*3+j*3+0]=0;
				edge2[i*m_nLine2Width*3+j*3+1]=0;
				edge2[i*m_nLine2Width*3+j*3+2]=0;
			}
	}
	int Threshold=5;
	BOOL flag=FALSE;
	for(int i=0;i<m_nLineHeight;i++)
		{
			for(int j=2;j<m_nLine1Width-2;j++)
			{	
				flag=((m_pFalseBackground1[i*m_nLine1Width*3+j*3-6]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3-5]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3-4]>Threshold)&&
					  (m_pFalseBackground1[i*m_nLine1Width*3+j*3-3]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3-2]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3-1]>Threshold)&&
					  (m_pFalseBackground1[i*m_nLine1Width*3+j*3+3]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3+4]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3+5]>Threshold)&&
					  (m_pFalseBackground1[i*m_nLine1Width*3+j*3+6]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3+7]>Threshold)&&(m_pFalseBackground1[i*m_nLine1Width*3+j*3+8]>Threshold));
				if(flag)
				{
					edge1[i*m_nLine1Width*3+j*3-6]=0;
					edge1[i*m_nLine1Width*3+j*3-5]=0;
					edge1[i*m_nLine1Width*3+j*3-4]=0;

					edge1[i*m_nLine1Width*3+j*3-3]=0;
					edge1[i*m_nLine1Width*3+j*3-2]=0;
					edge1[i*m_nLine1Width*3+j*3-1]=0;

					edge1[i*m_nLine1Width*3+j*3+0]=0;
					edge1[i*m_nLine1Width*3+j*3+1]=0;
					edge1[i*m_nLine1Width*3+j*3+2]=0;

					edge1[i*m_nLine1Width*3+j*3+3]=0;
					edge1[i*m_nLine1Width*3+j*3+4]=0;
					edge1[i*m_nLine1Width*3+j*3+5]=0;

					edge1[i*m_nLine1Width*3+j*3+6]=0;
					edge1[i*m_nLine1Width*3+j*3+7]=0;
					edge1[i*m_nLine1Width*3+j*3+8]=0;
				}
				else
				{
					edge1[i*m_nLine1Width*3+j*3+0]=m_pSubResult1[i*m_nLine1Width*3+j*3+0];
					edge1[i*m_nLine1Width*3+j*3+1]=m_pSubResult1[i*m_nLine1Width*3+j*3+1];
					edge1[i*m_nLine1Width*3+j*3+2]=m_pSubResult1[i*m_nLine1Width*3+j*3+2];
				}			
			}
			for(int j=2;j<m_nLine2Width-2;j++)
			{
				flag=((m_pFalseBackground2[i*m_nLine2Width*3+j*3-6]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3-5]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3-4]>Threshold)&&
					  (m_pFalseBackground2[i*m_nLine2Width*3+j*3-3]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3-2]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3-1]>Threshold)&&
					  (m_pFalseBackground2[i*m_nLine2Width*3+j*3+3]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3+4]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3+5]>Threshold)&&
					  (m_pFalseBackground2[i*m_nLine2Width*3+j*3+6]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3+7]>Threshold)&&(m_pFalseBackground2[i*m_nLine2Width*3+j*3+8]>Threshold));
				if(flag)
				{
					edge2[i*m_nLine2Width*3+j*3-6]=0;
					edge2[i*m_nLine2Width*3+j*3-5]=0;
					edge2[i*m_nLine2Width*3+j*3-4]=0;

					edge2[i*m_nLine2Width*3+j*3-3]=0;
					edge2[i*m_nLine2Width*3+j*3-2]=0;
					edge2[i*m_nLine2Width*3+j*3-1]=0;

					edge2[i*m_nLine2Width*3+j*3+0]=0;
					edge2[i*m_nLine2Width*3+j*3+1]=0;
					edge2[i*m_nLine2Width*3+j*3+2]=0;

					edge2[i*m_nLine2Width*3+j*3+3]=0;
					edge2[i*m_nLine2Width*3+j*3+4]=0;
					edge2[i*m_nLine2Width*3+j*3+5]=0;

					edge2[i*m_nLine2Width*3+j*3+6]=0;
					edge2[i*m_nLine2Width*3+j*3+7]=0;
					edge2[i*m_nLine2Width*3+j*3+8]=0;
				}
				else
				{
					edge2[i*m_nLine2Width*3+j*3+0]=m_pSubResult2[i*m_nLine2Width*3+j*3+0];
					edge2[i*m_nLine2Width*3+j*3+1]=m_pSubResult2[i*m_nLine2Width*3+j*3+1];
					edge2[i*m_nLine2Width*3+j*3+2]=m_pSubResult2[i*m_nLine2Width*3+j*3+2];
				}
			  }
		}
		//使用提取的轮廓和边缘检测后的结果进行对比从而去掉阴影的边缘
		Threshold=20;
		for(int i=0;i<m_nLineHeight;i++)
		{
			for(int j=0;j<m_nLine1Width;j++)
			{
				if(abs(edge1[i*m_nLine1Width*3+j*3+0]-m_pSubResult1[i*m_nLine1Width*3+j*3+0])<Threshold)
					edge1[i*m_nLine1Width*3+j*3+0]=0;
				else
					edge1[i*m_nLine1Width*3+j*3+0]=m_pSubResult1[i*m_nLine1Width*3+j*3+0];
				if(abs(edge1[i*m_nLine1Width*3+j*3+1]-m_pSubResult1[i*m_nLine1Width*3+j*3+1])<Threshold)
					edge1[i*m_nLine1Width*3+j*3+1]=0;
				else
					edge1[i*m_nLine1Width*3+j*3+1]=m_pSubResult1[i*m_nLine1Width*3+j*3+1];
				if(abs(edge1[i*m_nLine1Width*3+j*3+2]-m_pSubResult1[i*m_nLine1Width*3+j*3+2])<Threshold)
					edge1[i*m_nLine1Width*3+j*3+0]=0;
				else
					edge1[i*m_nLine1Width*3+j*3+2]=m_pSubResult1[i*m_nLine1Width*3+j*3+2];				
			}
			
			for(int j=0;j<m_nLine2Width;j++)
			{
				if(abs(edge2[i*m_nLine2Width*3+j*3+0]-m_pSubResult2[i*m_nLine2Width*3+j*3+0])<Threshold)
					edge2[i*m_nLine2Width*3+j*3+0]=0;
				else
					edge2[i*m_nLine2Width*3+j*3+0]=m_pSubResult2[i*m_nLine2Width*3+j*3+0];
				if(abs(edge2[i*m_nLine2Width*3+j*3+1]-m_pSubResult2[i*m_nLine2Width*3+j*3+1])<Threshold)
					edge2[i*m_nLine2Width*3+j*3+1]=0;
				else
					edge2[i*m_nLine2Width*3+j*3+1]=m_pSubResult2[i*m_nLine2Width*3+j*3+1];
				if(abs(edge2[i*m_nLine2Width*3+j*3+2]-m_pSubResult2[i*m_nLine2Width*3+j*3+2])<Threshold)
					edge2[i*m_nLine2Width*3+j*3+0]=0;
				else
					edge2[i*m_nLine2Width*3+j*3+2]=m_pSubResult2[i*m_nLine2Width*3+j*3+2];				
			}
		}

	
	m_pFlowParaStruc=m_pLineSignal->StatLineSignal(pDC,m_nLeftPos3,m_nRightPos3,m_nLeftPos4,m_nRightPos4,m_fSignal1,m_fSignal2,m_fSpace2,
													   m_nMaxErrorToler21,m_nMiniReliability21,m_nMaxErrorToler22,m_nMiniReliability22,
													   m_nLineHeight,edge1,edge2, *m_pdwPos,bIsShow,2,bIsShowSignal2);
   	m_pDib1->Empty();
	delete m_pDib1;	
	m_pDib2->Empty();	
	delete m_pDib2;

	m_pDib1=new CDib(CSize(m_nLine1Width,m_nLineHeight),24);
	m_pDib2=new CDib(CSize(m_nLine2Width,m_nLineHeight),24);

	m_pDib1->m_lpImage=edge1;//*/m_pSubResult1;
	m_pDib2->m_lpImage=edge2;//*/m_pSubResult2;

	if(bIsShow==TRUE)
	{		
		m_pDib2->Draw(pDC,CPoint(555+m_nRoadWay12Width,545),CSize(m_nLine2Width,m_nLineHeight),bIsShow);
		m_pDib1->Draw(pDC,CPoint(555+m_nRoadWay11Width,593),CSize(m_nLine1Width,m_nLineHeight),bIsShow);		
	}
	
	delete[] m_pFalseBackground1;
	m_pFalseBackground1=NULL;
	delete[] m_pFalseBackground2;
	m_pFalseBackground2=NULL;

	m_pDib1->Empty();
	delete m_pDib1;	
	m_pDib2->Empty();	
	delete m_pDib2;		

	delete[] edge1;
	edge1=NULL;
	delete[] edge2;
	edge2=NULL;

	return m_pFlowParaStruc;
}

void CImageDeal1::SetDetectLine1(CDC *pDC,int nLineNum,
								 int nHeightPos3,int nLeftPos3,int nRightPos3,int nWidthLine,BOOL bIsShow)
{	
	CPoint oriPoint;
	oriPoint=CPoint(50,20);
	CBrush brush1(RGB(255,0,0) );//red line	
	int temp;
	if(nLeftPos3>=nRightPos3)
	{
		temp=nLeftPos3;
		nLeftPos3=nRightPos3;
		nRightPos3=temp;
	}

	m_rect1.left=oriPoint.x+nLeftPos3;
	m_rect1.right=oriPoint.x+nRightPos3;	
	m_rect1.bottom=oriPoint.y+240-nHeightPos3;//	
	m_rect1.top=oriPoint.y+240-nHeightPos3-nWidthLine;//
	
	pDC->SelectObject(brush1);

//	if(bIsShow==TRUE)
		pDC->Rectangle(m_rect1);	
		
}

void CImageDeal1::SetDetectLine2(CDC *pDC,int nLineNum,
								 int nHeightPos4,int nLeftPos4,int nRightPos4,int nWidthLine,BOOL bIsShow)
{	
	CPoint oriPoint;
	oriPoint=CPoint(50,20);
	CBrush brush2(RGB(0,255,0));//green line
	int temp;
	if(nLeftPos4>=nRightPos4)
	{
		temp=nLeftPos4;
		nLeftPos4=nRightPos4;
		nRightPos4=temp;
	}

	m_rect2.left=oriPoint.x+nLeftPos4;
	m_rect2.right=oriPoint.x+nRightPos4;
	m_rect2.bottom=oriPoint.y+240-nHeightPos4;
	m_rect2.top=oriPoint.y+240-nHeightPos4-nWidthLine;

	pDC->SelectObject(brush2);

//	if(bIsShow==TRUE)
		pDC->Rectangle(m_rect2);	

}
