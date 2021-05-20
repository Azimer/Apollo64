void rspVmudh_nodest(short *vsrc,short val)
{
	int i;
	__int64 *acptr=rsp_reg.accum;
	__int64 tmp;
	for(i=0;i<8;i++)
	{
		tmp=(__int64)(int)*(vsrc++)*(__int64)(int)val;
		tmp<<=32;
		*(acptr++)=tmp;
	}
}

void rspVmadh_nodest(short *vsrc,short val)
{
	int i;
	__int64 *acptr=rsp_reg.accum;
	__int64 tmp;
	for(i=0;i<8;i++)
	{
		tmp=(__int64)(int)*(vsrc++)*(__int64)(int)val;
		tmp<<=32;
		*(acptr++)+=tmp;
	}
}

void rspVmudn_nodest(WORD *vsrc,short val)
{
	int i;
	__int64 *acptr=rsp_reg.accum;
	unsigned __int64 tmp;
	DWORD a,b;
	for(i=0;i<8;i++)
	{
		a=*(vsrc++);
		b=val;
		tmp=a*b;
		tmp<<=16;
//		tmp=(__int64)((int)*(vsrc++)*(int)val)<<16;
		*(acptr++)=tmp;
	}
}

void rspVmadh(short *vdst,short *vsrc,short val)
{
	int i;
	__int64 *acptr=rsp_reg.accum;
	__int64 tmp1,tmp2;
	for(i=0;i<8;i++)
	{
		tmp1=((__int64)*(vsrc++)*(__int64)val);
		tmp1<<=32;
		*(acptr)+=tmp1;
		tmp2=*(acptr++);
		tmp2>>=32;
		if(tmp2>32767) tmp2=32767;
		else if(tmp2<-32768) tmp2=-32768;
		*(vdst++)=(short)tmp2;
	}
}

void rspVsaw(short *dst,BYTE mode)
{
	int i;
	__int64 *acptr=rsp_reg.accum;
	switch(mode)
	{
	case 8:
		{
			for(i=0;i<8;i++)
			{
				*(dst++)=(short)(*(acptr++)>>48);
			}
			break;
		}
	case 9:
		{
			for(i=0;i<8;i++)
			{
				*(dst++)=(short)(*(acptr++)>>32);
			}
			break;
		}
	case 0xa:
		{
			for(i=0;i<8;i++)
			{
				*(dst++)=(short)(*(acptr++)>>16);
			}
			break;
		}
	case 0xb:
		{
			for(i=0;i<8;i++)
			{
				*(dst++)=(short)(*(acptr++)>>0);
			}
			break;
		}
	}
}