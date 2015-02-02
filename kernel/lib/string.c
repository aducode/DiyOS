//
//#include <stdio.h>
//void itoa(int value, char * string, int radix);
//int main(int argv, char ** argc)
//{
//	char str[256];
//	itoa(20, str, 10);
//	printf("%s\n",str);
//}
void itoa(int value, char * string, int radix)
{
	int len=0,i,t;
	char *s=string, tmp;
	while(value>0)
	{
		t=value%radix;
		if(t<10){
			*s = (char)((int)'0' + t);
		} else {
			//16进制中的A-F
			*s = (char)((int)'A'+ t-10);
		}
		value /= radix;
		len++;
		s++;
	}
	s=string;
	for(i=0;i<len/2;i++)
	{
		tmp = *(s+i);
		*(s+i)=*(s+len-1-i);
		*(s+len-1-i)=tmp;
	}
	if(len<=0){
		*(s+len++)='0';
	}
	switch(radix)
	{
		case 16:
			*(s+len++)='H';
			break;
		case 8:
			*(s+len++)='O';
			break;
		case 2:
			*(s+len++)='B';
			break;
		default:
			break;
			//pass
	}
	*(s+len)='\0';
}




