// ****************************************************************
// *                                                              *
// *       Wave to Bin                                            *
// *    für den Casio FX-602P                                     *
// *                                                              *
// * Wandelt eine Wavedatei, die mit dem                          *
// * Casio-Interface gespeichert wurde, in einen                  *
// * lesbaren ASCII Code                                          *
// *                                                              *
// *  von: Arno Schweißinger, Wanne-Eickel                        *
// *                                                              *
// *  Version vom:    13.04.2005                                  *
// *                                                              *
// *                                                              *
// *                                                              *
// ****************************************************************

unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Wave, ExtCtrls, ComCtrls,  Buttons, ToolWin, Math,
  Spin, Menus, MPlayer;

type
  TForm1 = class(TForm)
    OpenDialog1: TOpenDialog;
    StatusBar1: TStatusBar;
    ScrollBar1: TScrollBar;
    Panel1: TPanel;                   
    Panel4: TPanel;
    Image1: TImage;
    Trigger: TTrackBar;
    Memo2: TMemo;
    ToolBar1: TToolBar;
    PopupMenu1: TPopupMenu;
    SpeedButton1: TSpeedButton;
    SpeedButton3: TSpeedButton;
    SpeedButton2: TSpeedButton;
    Panel5: TPanel;
    RichEdit1: TRichEdit;
    MediaPlayer1: TMediaPlayer;
    Splitter1: TSplitter;
    Splitter2: TSplitter;
    Panel3: TPanel;
    Label3: TLabel;
    Zoom: TTrackBar;
    Label1: TLabel;
    procedure Button1Click(Sender: TObject);
    procedure ScrollBar1Scroll(Sender: TObject; ScrollCode: TScrollCode;
      var ScrollPos: Integer);
    procedure TriggerChange(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure TextOutClick(Sender: TObject);
    procedure MediaPlayer1Click(Sender: TObject; Button: TMPBtnType;
      var DoDefault: Boolean);
  private
    { Private-Deklarationen }
  public
    { Public-Deklarationen }
  end;

const
  Version     = 'Version 13.04.2005';
  EndeCode    =  $FF; // Letztes Token
  nStartBit   =    1;
  nDatenBit   =    8;
  nParity     =    1;
  nStopBit    =    2;
  nWortLaenge = nStartBit+nDatenBit+nParity+nStopBit;
  SyncWaves   =   50; // Dauer des Lead-in, danach dürfen Daten kommen
  Baud        =  300; // Übertragungsgeschwinigkeit
  BaseFreqLow = 1200; // 1.2 kHz Low  Signal
  BaseFreqHi  = 2400; //         High Signal
  CodeStart   =   -8; // beginnt mit einem Header
  CodeEnde    = 1023; // Läenge der Liste, 512 Steps sind zu wenig
  Auswertung  = '%3d : %s %3x %s';

type                         { Aufbau Wave}
  TWaveInfo = record
    Start, ende, freq, bitLen, count, typ, sum, Error    : LongInt; end;
  TTokenListe = Record
   Token:String; Alpha:Boolean; ZlUmBru:Boolean   ; end;
  TCode = Record
    Token:Word;  Alpha:Boolean; ZlUmBru:Boolean   ; end;

var
  Form1        : TForm1;
  WaveStruktur : TWaveStruktur;
  WaveFile     : File;
  Buffer       : Char;
  WaveInfo     : TWaveInfo;
  Alpha        : Boolean;                           // Textmodus
  TokenListe   : Array [0..EndeCode,  Boolean] of TTokenListe;
  SourceCode   : Array [CodeStart .. CodeEnde] of TCode;

implementation

{$R *.dfm}

procedure WaveAusgabeLoeschen;
begin
  Form1.Image1.Canvas.Pen.Color:=clBlack;
  // Rahmen drumm
  Form1.Image1.Canvas.Rectangle(0,0,Form1.Image1.Width,Form1.Image1.Height);
  // Null Linie einzeichnen
  Form1.Image1.Canvas.MoveTo(0, (Form1.Image1.Height div 2));
  Form1.Image1.Canvas.LineTo(Form1.Image1.Width, (Form1.Image1.Height div 2));
  // Trigger einzeichnen
  Form1.Image1.Canvas.Pen.Color:=clRed;
  Form1.Image1.Canvas.MoveTo(0, (Form1.Image1.Height div 2)+ Form1.Trigger.Position);
  Form1.Image1.Canvas.LineTo(Form1.Image1.Width, (Form1.Image1.Height div 2)+ Form1.Trigger.Position);
  Form1.Image1.Canvas.MoveTo(0, (Form1.Image1.Height div 2)- Form1.Trigger.Position);
  Form1.Image1.Canvas.LineTo(Form1.Image1.Width, (Form1.Image1.Height div 2)- Form1.Trigger.Position);
  // Pen in die Nullposition
  Form1.Image1.Canvas.Pen.Color:=clBlack;
  Form1.Image1.Canvas.MoveTo(0, (Form1.Image1.Height div 2));
end;

procedure TForm1.Button1Click(Sender: TObject);
begin
  Opendialog1.InitialDir:= GetCurrentDir+'\wav';
  OpenDialog1.Filter := 'Wavedateien (*.wav)|*.WAV|alle Dateien (*.*)|*.*';
  if Opendialog1.Execute then
  begin
    RichEdit1.Clear;
    Memo2.Clear;
    Caption:='Wav2Bin '+Opendialog1.FileName;
    WaveStruktur := ReadWaveStruktur(Opendialog1.FileName);
    if WaveStruktur.Error=0 then
    begin
//      Memo1.Text:=    'ChunkID           '+WaveStruktur.ChunkID;
//      Memo1.Lines.Add('ChunkSize         '+InttoStr(WaveStruktur.ChunkSize));
//      Memo1.Lines.Add('ChunkFormat       '+WaveStruktur.ChunkFormat);
//      Memo1.Lines.Add('SubChunkID        '+WaveStruktur.SubChunkID);
//      Memo1.Lines.Add('SubChunkSize      '+InttoHex(WaveStruktur.SubChunkSize, 8)+'h');
//      Memo1.Lines.Add('AudioFormat       '+InttoHex(WaveStruktur.AudioFormat,  4)+'h');
      Statusbar1.Panels[0].Text:=('Kanäle: '+IntToStr(WaveStruktur.Channels));
      Statusbar1.Panels[1].Text:=('Abtastrate: '+InttoStr(WaveStruktur.SamplesPerSec));
//      Memo1.Lines.Add('AvgBytesPerSec    '+InttoStr(WaveStruktur.AvgBytesPerSec));
//      Memo1.Lines.Add('ByteRate          '+InttoHex(WaveStruktur.ByteRate,     4)+'h');
      Statusbar1.Panels[2].Text:=('Abtastgröße: '+InttoStr(WaveStruktur.BitsPerSample));
//      Memo1.Lines.Add('DataSubchrunkID   '+WaveStruktur.DataSubchrunkID);
//      Memo1.Lines.Add('DataSubchrunkSize '+InttoStr(WaveStruktur.DataSubchrunkSize));
      Application.ProcessMessages;
      if WaveStruktur.BitsPerSample<>8 then begin
        ShowMessage('Abtastgröße ist nicht 8 bit!'); exit; end;
      if WaveStruktur.SamplesPerSec<>11025 then begin
        ShowMessage('Abtastrate ist nicht 11025 bit/s!'); exit; end;
      if WaveStruktur.Channels<>1 then begin
        ShowMessage('Wave ist nicht in Mono!'); exit; end;

      ScrollBar1.Min:= 0;
      ScrollBar1.Max:= WaveStruktur.DataSubchrunkSize-Image1.Width;
      ScrollBar1.PageSize:= ScrollBar1.Width div(WaveStruktur.DataSubchrunkSize div Image1.Width);
      ScrollBar1.LargeChange:= Image1.Width;
      TriggerChange(Sender);
      Application.ProcessMessages;
    end;  // WaveStruktur.Error=0
  end;   // Opendialog1.Execute
  SpeedButton3.Enabled:=true;

  MediaPlayer1.DeviceType := dtWaveAudio;
  MediaPlayer1.Open;
end;

procedure TForm1.TriggerChange(Sender: TObject);
var
  i: Integer;
begin
//  Label1.Caption:= IntToStr(Trigger.Position);
  Form1.Statusbar1.Panels[3].Text:='Trigger : '+IntToStr(Trigger.Position);
  Form1.Statusbar1.Panels[4].Text:='Zoom : '+IntToStr(Zoom.Position)+'x';
  i:=1;
  ScrollBar1Scroll(Sender,scPosition,i);
end;

procedure TForm1.ScrollBar1Scroll(Sender: TObject; ScrollCode: TScrollCode;
  var ScrollPos: Integer);
Var
  i, xPos : Integer;
  Buffer  : Word;
begin
  WaveAusgabeLoeschen;
  If not FileExists(Form1.Opendialog1.FileName) then exit;
  OpenWave(Form1.Opendialog1.FileName);
  xPos:=0;
  Image1.Canvas.Pen.Color:=clBlack;
  for i:=WaveStruktur.DataStartAdress + ScrollPos to WaveStruktur.DataEndAdress-1 do
  begin
    Buffer:=ReadWordFromFile(i);
      Image1.Canvas.LineTo(xPos*Zoom.Position, (Image1.Height div 2)+ Buffer-$80);
    If xPos*zoom.Position >= Image1.Width then break;
    Inc(xPos);
  end;
  CloseWave;
end;

function FindeEineWelle(WInfo:TWaveInfo):TWaveInfo;
var
  Pegel  : Word;
  LHPegel: integer;
  i, Nulldurchgang      : integer;
begin
  LHPegel:=1;  Nulldurchgang:=0;
  for i:=WInfo.Start to WInfo.ende do
  begin
    Pegel  := ReadWordFromFile(i+WaveStruktur.DataStartAdress);
    If  LHPegel = 1 then                                   // Positive Halbwelle
    begin
      if Pegel>$80 then
        continue                                           // es ändert sich nichts
      else
      begin                                                // Wechsel von positiv nach negativ
        LHPegel:=-1;
        Inc(Nulldurchgang);
      end;
    end
    else                                                   // Negatitive Halbwelle
    begin
      if Pegel<$80 then
        continue                                           // es ändert sich nichts
      else
      begin
        LHPegel:=1;                                        // Wechsel von negativ nach positiv
        Inc(Nulldurchgang);
        Inc(WInfo.sum);                                    // zäht die Anzahl der Wellen
      end;
    end;
    If WInfo.count = Nulldurchgang then break;             // 3 mal Nullduchlauf gleich eine Sinuskuve
  end;  // for
  WInfo.ende:= i;
  Result    :=WInfo;
end;

Function FindeTriggerlevel:Integer;
var
  i : Integer;
begin
  for i:=WaveStruktur.DataStartAdress to WaveStruktur.DataEndAdress-1 do
    If Abs(ReadWordFromFile(i)-$80)>=Form1.Trigger.Position then break;
  If i >= WaveStruktur.DataStartAdress+WaveStruktur.DataSubchrunkSize-1 then
        Result := -1
  else  Result :=  i;
  Form1.Statusbar1.Panels[6].Text:='TriggerStartPos : '+IntToStr(Result);
end;

Function SkipLeadin(WInfo:TWaveInfo):TWaveInfo;
begin
  WInfo.ende := FindeTriggerlevel+1;
  WInfo.count:= 3;
  repeat
    repeat   // Eine Welle
      WInfo.Start:= WInfo.ende - 1;
      WInfo.Ende := WInfo.Start+15;
      WInfo      := FindeEineWelle(WInfo);
    Until (WInfo.Ende-WInfo.Start) > 6;
  Until  WInfo.sum > SyncWaves;
  Result:=WInfo;
end;

procedure TForm1.Button2Click(Sender: TObject);
Var
  Byts, BitSum,BitSumAlt, counter : integer;
  sCode : string;
begin
  Memo2.Clear;
  Memo2.Lines.Add('Step: Bin      Hex Code');
  If not FileExists(Form1.Opendialog1.FileName) then exit;
  OpenWave(Opendialog1.FileName);
  WaveInfo.sum:=0;
  Byts:=CodeStart;
  WaveInfo:= SkipLeadin(WaveInfo);
  WaveInfo.count:= 9;                                      // 4 Vollwellen
  Alpha := False;
  repeat
    counter:=0;
    BitSumAlt:=BitSum; BitSum:=0;                          // Abbruchbedingung $FF folgt $00
    repeat
      WaveInfo.Start:=WaveInfo.ende-1;
      WaveInfo.ende:= WaveInfo.Start+45;                   // Block der bearbeitet wird
      WaveInfo:=FindeEineWelle(WaveInfo);
      If WaveInfo.ende-WaveInfo.Start<27 then              // ist High Signal   27=(WaveStruktur.SamplesPerSec/(BaseFreqHi+BaseFreqLow)/2)
      begin   //HIGH  Wert
        BitSum:=BitSum shl 1+1;
        WaveInfo.Start:=WaveInfo.ende-1;                   // fehlende 4 Vollwellen verwerfen
        WaveInfo.ende:= WaveInfo.Start+45;
        WaveInfo:=FindeEineWelle(WaveInfo);
      end
      else
      begin   // LOW  Wert
        BitSum:=BitSum shl 1;
      end;
      inc(counter);
    until Counter=nWortLaenge;                             // Wort ist erkannt
    Inc(Byts);                                             // Zähler für Programmschritte
    BitSum:=BitSum shr 3;                                  // 1 Parity und 2 StopBit verwerfen
    BitSum:=TauschBitfolge(BitSum);                        // byte von hinten nach vorne lesen
    if BitSum=$2B then Alpha:= Not Alpha;                  // " im code
    If Byts>0 then
      sCode:=TokenListe[BitSum,Alpha].Token;
    SourceCode[Byts].Token:=BitSum;
    SourceCode[Byts].Alpha:=Alpha;
    Memo2.Lines.Add(Format(Auswertung,[Byts,WordToBin(BitSum),BitSum,sCode]));
    If Byts>=CodeEnde then break;                          // Überlauf Datei wird nicht richtig erkannt
  until (BitSumAlt=$FF) and (BitSum=$00) and (Byts>8);
  closeWave;
  TextOutClick(Sender);
end;

Procedure LoadCode(FileName:String);
var
  CodeToken  : TStringlist;
  CodeListe  : TStringlist;
  I : integer;
begin
  CodeListe:=TStringlist.Create;  CodeToken:=TStringlist.Create;
  CodeListe.LoadFromFile(FileName);
  for i:=0 to CodeListe.Count-1 do
  begin
    CodeToken.CommaText:=CodeListe.Strings[i];
    if CodeToken[0] <> '' then
    begin
      TokenListe[StrToInt(CodeToken[0]),Alpha].Token:=CodeToken[2];
      if CodeToken[1]='x' then     // prüfe Zeilenumbruch
        TokenListe[StrToInt(CodeToken[0]),Alpha].ZlUmBru:=true;
    end
  end;
  CodeListe.Destroy; CodeToken.Destroy;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  caption := Caption +' '+Version;
  Alpha := False;
  LoadCode(GetCurrentDir+'\Daten\code.csv');
  Alpha := True;
  LoadCode(GetCurrentDir+'\Daten\Alpha.csv');
  SpeedButton3.Enabled:=false;
end;

procedure TForm1.TextOutClick(Sender: TObject);
var
  Zeile : String;
  i : integer;
begin
  If SourceCode[0].Token=$00 then exit;      // Ende erreicht
  For I:=1 to High(SourceCode) do
  begin
    if SourceCode[i].Token=$FF then break;
    If SourceCode[i].Alpha then
    begin
      Zeile := Zeile + TokenListe[SourceCode[i].Token,SourceCode[i].Alpha].Token;
    end
    else
    begin
      Zeile := Zeile + TokenListe[SourceCode[i].Token,SourceCode[i].Alpha].Token+' ';
    end;
    if TokenListe[SourceCode[i].Token,SourceCode[i].Alpha].ZlUmBru then
      Zeile := Zeile + #10;
  end;
  RichEdit1.Text:=Zeile;
end;

procedure TForm1.MediaPlayer1Click(Sender: TObject; Button: TMPBtnType;
var DoDefault: Boolean);
begin
  With MediaPlayer1 do
  begin
     Filename := Opendialog1.FileName; //Wavedatei auswählen
     DeviceType := dtAutoSelect; //Gerätekompatibilität für WAV einrichten
     VisibleButtons := [btPlay, btStop]; //Begrenzung auf zwei Schaltflächen
     Open; //Wiedergabe
     Play;
  end;
end;

end.













