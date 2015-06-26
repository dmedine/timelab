/* this file belongs to */
/* timelab-0.10 by David Medine */

// this source code defines the 
// argument parsing in timelab

#include "tl_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


static float do_mantissa(char *float_chars, int len){
  
  float mantissa = 0.0;
  int i=0, j=0;
  float mult = 0.0;

  for(i=0;i<len;i++)
    {
      mult = .1;
      for(j=0;j<i;j++)
	mult*=.1;
      mantissa+=mult*(float_chars[i]-'0');
    }
  return mantissa;

}

static int get_number(tl_arg *arg, char *num_chars, int len){

  int ptr = 0;
  int scale = 1;
  int i_out = 0;
  int i, j;
  float mantissa, f;
  int int_len = 0;
  int ec = 1;
  int is_float = 0;
  int is_neg = 0;
  int mult;

  if(num_chars[ptr]=='-'){scale = -1; is_neg = 1; ptr++;}
  while(ptr<len && num_chars[ptr]!='.')
    {
      int_len++;
      ptr++;
      if(num_chars[ptr]=='.')
	{
	  mantissa = do_mantissa(num_chars+ptr+1, len-int_len-1-is_neg);	  is_float = 1;
	}
    }

  for(i=0+is_neg; i<int_len+is_neg; i++)
    {
      mult = 1;
      for(j=0; j<int_len-i-1+is_neg;j++)
	  mult*=10;
 
      i_out += mult * (num_chars[i]-'0');
    }

  if(is_neg==1)i_out*=-1;

  if(is_float == 1)
    {
      if(is_neg==1)mantissa*=-1.0;
      f = (float)i_out+mantissa;
      arg->type = A_FLOAT;
      arg->f_val = f;

    }
  else{arg->type = A_INT;arg->i_val = i_out;}
  return ec;

}

static void got_one(tl_arglist *args, char *buff, int len){
  
  tl_arg_t type;
  int i;
  int foo;
  char *chars;
  tl_arg_t arg;
  chars = (char *)malloc(sizeof(char) * (len+1));


  for(i=0;i<len; i++)
    chars[i] = buff[i];

  // alocate memory for the current arg
  args->argv[args->argc] = (tl_arg *)malloc(sizeof(tl_arg));
  if(chars[0]>='0'&&chars[0]<='9'||chars[0]=='-'&&chars[1]>='0'&&chars[1]<='9')
    {
      //printf("is int, %d\n", len);  
      foo = get_number(&args->argv[args->argc++], chars, len);
    }
  else
    { 
      //  printf("is str\n");
      // simply write the string into a buffer
      args->argv[args->argc]->type = A_STR;
      chars[i] = '\0';
      strcpy(args->argv[args->argc++]->str_val, chars); 
    }

  //printf("got one: %s\n", chars);
  free(chars);
}

void tl_parse_args(tl_arglist *x, const char *arg_str){

  char buff[256], *bp = buff;
  int wrt_pt = 0, rd_pt = 0;
  int token_len = 0;

  while(arg_str[rd_pt]!='\0'){
    bp[wrt_pt]=arg_str[rd_pt++];
    if(bp[wrt_pt]!=' '){wrt_pt++;token_len++;}
    else
      {
	got_one(x, bp, token_len);
	wrt_pt = 0;
	token_len = 0;
      }
    
  }
  //at the end, one more to do, but back it up from the loop's end
  got_one(x, bp, token_len);

}

