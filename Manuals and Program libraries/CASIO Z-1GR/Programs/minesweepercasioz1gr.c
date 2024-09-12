#define dimx 6
#define dimy 6
#define tx 190
#define ty 30
#define pct 5
int _s1=1150, _s2=12145, _s3=28000,_p1=0,_p2=0,_p3=0 ;
void main()
{int rc=0;
int debut();
clrscr();
printf("Donnez un chiffre entre 0 et 9 pour initialiser le tirage des griles:");
do {rc=getch();}
while (rc <48 || rc>57);
_s1=_s1*(1+rc)/100;
printf("%d", rc);
debut(1);}
int debut(coups);
int coups;
{int lx,ly;
/* nb de cases */
int cases[dimx][dimy];/*cases */
int *p_c;
double rand();/* Nbre aleat */
void grille();/* Imp grille */
void genere();/* Gener mines*/
void calcul();/* Calcul nb  */
void rect();  /* Rectangle  */
void delrect();/*efface rect*/
void jeu();
void select();
void deselect();
void perdu(); /* explosion */
void decouvre(); /* pas mine */
void box();
void etendre ();
int verif();
void adjacente();
/* initialisations */
clrscr();
p_c=&cases[0][0];
/* largeur -hauteur */
lx=(tx/dimx);
ly=(ty/dimy);
/* dessine grille */
grille(lx,ly);
/* genere mines */
genere(p_c);
/* densite des mines */
calcul(cases);
/* Le jeu */
jeu(cases,lx,ly,coups);
 return 1;
}
/* impression de la grille */
void grille(lx,ly);
int lx, ly;
{int x,y ;
for (x=0;x<dimx;x++)
for (y=0;y<dimy;y++)
{rect(x*lx+1+1,y*ly+1+1,lx-2,ly-2);};
}
/* generation des mines */
void genere(p_c);
int *p_c;
{int x,y ;
double l=0;
_p1=_s1;_p2=_s2;_p3=_s3;
for (x=1;x<=dimx;x++)
for (y=1;y<=dimy;y++)
{l=rand();
/*printf("(%d,%d):%f-",x,y,l);*/
if (l<0.2) {
*p_c=9;
}
    else {*p_c=0;}
p_c++;};
};
/* calcul nb de mines */
void calcul(cases);
int cases[dimx][dimy];
{int x,y,nb;
for (x=0;x<dimx;x++)
for (y=0;y<dimy;y++)
{nb=0;
if (x>0){
if (y>0) {if (cases[x-1][y-1]==9) {nb+=1;}}
if (cases[x-1][y]==9) {nb+=1;}
if (y<dimy-1) {if (cases[x-1][y+1]==9) {nb+=1;}}
}
if (y>0) {
if (cases[x][y-1]==9) {nb+=1;}
if (x<dimx-1) {if (cases[x+1][y-1]==9) {nb+=1;}}
}
if (x<dimx-1) {
if (cases[x+1][y]==9) {nb+=1;}
if (y<dimy-1) {if (cases[x+1][y+1]==9) {nb+=1;}}
}
if (y<dimy-1) {if (cases[x][y+1]==9) {nb+=1;}}
if (cases[x][y]!=9) {cases[x][y]=nb;};
};
};
/* rectangle */
void rect(x2,y2,lx2,ly2);
int x2,y2,lx2,ly2;
{box(x2,y2,x2+lx2,y2+ly2,0);
/*line (x2,y2,x2+lx2,y2);
line (x2+lx2,y2,x2+lx2,y2+ly2);
line (x2+lx2,y2+ly2,x2,y2+ly2);
line (x2,y2+ly2,x2,y2);*/
};
/* efface rectangle */
void delrect(x2,y2,lx2,ly2);
int x2,y2,lx2,ly2;
{box (x2,y2,x2+lx2,y2+ly2,1);
/*linec (x2+lx2,y2,x2+lx2,y2+ly2);
linec (x2+lx2,y2+ly2,x2,y2+ly2);
linec (x2,y2+ly2,x2,y2);*/
};

 /* Nombre aléatoire */
double rand()
{double r;
_s1=(_s1%177)*171-(_s1/177)*2;
if (_s1<0){_s1+=30269;}
_s2=(_s2%176)*172-(_s2/176)*35;
if (_s2<0){_s2+=30307;}
_s3=(_s2%178)*170-(_s3/178)*63;
if (_s3<0){_s3+=30323;}
r=_s1/30269.0+_s2/30307.0+_s3/30323.0;
while (r>1.0) r-=1.0 ;
return r;
}
/* le jeu */
void jeu(cases,lx,ly,coups);
int cases[dimx][dimy],lx,ly,coups;
{int nb=0,key=0,px,py,x,y;
px=dimx/2;
py=dimy/2;
select(px,py,lx,ly);
while (nb<1000)
{if (verif(cases)==0){
clrscr();printf("Bravo !\nVous avez gagne en %d coups!\nPressez ENTREE pour
rejouer.",coups);key=getch();debut(1);}
key=getch();
deselect(px,py,lx,ly);
if (key==29 && px>0){px=px-1;}
if (key==28 && px<dimx-1){px=px+1;}
if (key==30 && py>0){py=py-1;}
if (key==31 && py<dimy-1){py=py+1;}
/* touche S pour decouvrir */
if (key==115 && cases[px][py]!=9){etendre(cases,px,py,lx,ly);}
if (key==115 && cases[px][py]==9) {perdu(px,py,coups);}
/* touche M pour marquer une mine */
if (key==109) {
x=px*lx+lx/2;
y=py*ly+ly/2;
line (x-7,y+2,x+7,y+2);line (x-7,y+1,x+7,y+1);cases[px][py]=cases[px][py]+90;}
nb++;
select(px,py,lx,ly);
/*gotoxy(30,0);
printf("%d",cases[px][py]);*/
}}
void select(px,py,lx,ly);
int px,py,lx,ly;
{rect(px*lx+1,py*ly+1,lx,ly);}
void deselect(px,py,lx,ly);
int px,py,lx,ly;
{delrect(px*lx+1,py*ly+1,lx,ly);
}
void perdu(px,py,coups)
int px,py,coups;
{int key;
clrscr();
gotoxy(0,0);
printf ("La case (%d,%d) etait une mine.\nC'est perdu.",px+1,py+1);
printf ("Pressez R pour Recommencer, C\npour Changer et Q pour Quitter.");
key=getch();
if (key==99){debut(1);}
if (key==114){_s1=_p1;_s2=_p2;_s3=_p3;debut(coups+1);}
if (key==113){clrscr();printf("A bientot.");exit();}}
void etendre(cases,px,py,lx,ly);
int cases[dimx][dimy],px,py,lx,ly;
{if (px>=0 && py>=0 && px<dimx && py <dimy)
/* CAS 1 : pas de mine adjacente */
{if (cases[px][py]==0 || cases[px][py]==90) {
delrect(px*lx+2,py*ly+2,lx-2,ly-2);

 cases[px][py]=cases[px][py]+80;
etendre(cases,px-1,py-1,lx,ly);
etendre(cases,px-1,py,lx,ly);
etendre(cases,px-1,py+1,lx,ly);
etendre(cases,px,py-1,lx,ly);
etendre(cases,px,py+1,lx,ly);
etendre(cases,px+1,py-1,lx,ly);
etendre(cases,px+1,py,lx,ly);
etendre(cases,px+1,py+1,lx,ly);
}
else
{/* CAS 2 : mine adjacente */
/*if (cases[px][py]<4) {*/
adjacente(cases,px,py,lx,ly);
/*}*/
}}}
/* CAS 2 : mine adjacente */
void adjacente(cases,px,py,lx,ly);
int cases[dimx][dimy],px,py,lx,ly;
{int x,y;
x=px*lx+lx/2;
y=py*ly+ly/2+1;
linec (x-7,y,x+7,y);linec (x-7,y+1,x+7,y+1);
if (cases[px][py]>=90) {cases[px][py]=cases[px][py]-90;}
if (cases[px][py]>=80) {cases[px][py]=cases[px][py]-80;}
if (cases[px][py]==1) {line (x-1,y,x+1,y);}
if (cases[px][py]==2) {line (x-3,y,x-1,y);line (x+1,y,x+3,y);}
if (cases[px][py]==3) {line (x-5,y,x-3,y);
line (x-1,y,x+1,y);line (x+3,y,x+5,y);}
if (cases[px][py]>=4 && cases[px][py]<8) {line (x-7,y,x-5,y);line (x-3,y,x-1,y);
line (x+1,y,x+3,y);line (x+5,y,x+7,y);}
if (cases[px][py]<9) {cases[px][py]=cases[px][py]+80;}
}
int verif(cases);
int cases[dimx][dimy];
{int nb=0,x,y;
for (x=0;x<dimx;x++)
for (y=0;y<dimy;y++)
{if (cases[x][y] <= 8 || cases[x][y]>=89 && cases[x][y]<=98){nb+=1;}
}return nb;}
void box(x,y,z,t,d);
int x,y,z,t,d;
{pokeb(0,0x2003,x);
pokeb(0,0x2004,y);
pokeb(0,0x2005,z);
pokeb(0,0x2006,t);
pokeb(0,0x2007,d);
pokeb(0,0x2008,20);
call (8192,0);
}

ORG 2000H
    JMP START
XD  DB  0
YD  DB  0
XF  DB  0
YF  DB  0
D   DB  0
E   DB  0
START:
    MOV BL,XD
    MOV BH,YD
    MOV CL,XF
    MOV CH,YF
    MOV DL,D
    MOV AH,E

 INT 41H IRET
