#!/home/wolfcreek/toshi/bin/perl

use Socket;
$HISTFILE = "/tmp/FvConHist0";

$tty = `tty`; 
$tty =~ s/\n//;

$ESC = "\c[";
# needs these lines in .Xdefault to make home and end key work 
# xterm*VT100*Translations: #override \n \
#	<Key> Home: string(0x1b) string("[214z" ) \n \
#	<Key> End:  string(0x1b) string("[220z" ) \n 

#---------------- change this block for key binding -----------------
$Func{"$ESC\[214z"} = 'bol';  #home key 
$Func{"$ESC\[220z"} = 'eol';  #end key
$Func{"$ESC\[A"}= 'prev-line'; #up
$Func{"$ESC\[B"}= 'next-line'; #down
$Func{"$ESC\[C"}= 'next-char'; #right
$Func{"$ESC\[D"}= 'prev-char'; #left
$Func{"${ESC}f"}= 'next-word'; 
$Func{"${ESC}b"} = 'prev-word';

$Func{"\cD"} = 'del-char';
$Func{"\c?"} = 'del-char';
$Func{"\cH"} = 'bs';
$Func{"\cv"} = 'quote';
$Func{"\cU"} = 'del-line';
$Func{"\cR"} = 'search-rev';
$Func{"\cK"} = 'clr-eol';
$Func{"\ca"} = 'bol';
$Func{"\ce"} = 'eol';
$Func{"\cp"} = 'prev-line';
$Func{"\cn"} = 'next-line';
$Func{"\cf"} = 'next-char';
$Func{"\cb"} = 'prev-char';
$Func{"\cw"} = 'del-word';
$Func{"\xE6"} = 'next-word'; # alt-f
$Func{"\xE2"} = 'prev-word'; # alt-b
$Func{"${ESC}b"} = 'prev-word'; # esc-b
$Func{"${ESC}f"} = 'next-word'; # esc-f
$Func{"${ESC}>"} = 'eoh-ign-mode'; # end of history, ignore mode
$Func{"${ESC}<"} = 'boh-ign-mode'; # begining of history, ignore mode

$KEY_EOF = "\cD"; #eof only with empty line
#---------------- end of key binding -----------------

$TERM_EEOL = "$ESC\[K";		# erase to end of line
$TERM_RIGHT = "$ESC\[C";      # move cursor right
$TERM_UP = "$ESC\[A";        # move cursor up

@Hist = ();
@Histall = ();
$HIST_SIZE = 50;

main();
exit;

sub main {
	my($sun, $line, $cmd);

	my $SOCKET_NAME = "/tmp/FvConSocket";
	socket(SH, PF_UNIX, SOCK_STREAM, 0) || die "$! ";
	$sun = sockaddr_un($SOCKET_NAME);
	connect(SH,$sun) || die $!;

	if( $child = fork()  ) {
		&input_open($tty,$tty,$HISTFILE,1);
		while( $cmd = &input('','',1) ) {
			next if $cmd =~/^\s*$/;
			last if $cmd eq '\0';
			send( SH, $cmd,0 );
		}
		dokill();
	}
	#child handles output
	while($line =<SH>) {
		last if $line[0] eq '\0';
		print "$line\n";
	}
	unlink SH;
	kill -9, getppid() ; 
}

sub dokill {
	unlink SH;
	kill -9,$child if $child;
	exit;
}

sub input_open {
	# arg0 input device
	# arg1 output device
	# arg2 history file
	# arg3 key selection - bit0 
	#                      bit1 
	#                      bit2 return undef esc code as it is
	
	($Dev_in,$Dev_out,$File,$Ksel) = @_;
	if( !$Dev_in ) {$Dev_in = $tty;}
	elsif( $Dev_in eq "not a tty" ) { $Dev_in = $ENV{'TTY'};}
	if( !$Dev_out ) {$Dev_out = $tty;}
	if( !$File ) { $File = '/tmp/input.tmp';}
	open(IN,"<$Dev_in") || die "open in at input_open '$Dev_in' $!";
	open(OUT,">$Dev_out") || die "can't open input at 'input_open' $!";
	select((select(OUT), $| = 1)[0]); # unbuffer pipe                
	if( defined $File ) { 
		if( open(INITF,"$File") ) { 
			do "$File"; 
			@Histall=<INITF>; close(INITF); $#Histall--;
		}else{
			print STDERR "Can't open history file $File\n";
		}
	}
}

sub input_close  {
	close(IN);
	close(OUT);
}

sub escape  {
	local($c);
	local($s) = "";
	do  {
		$c = getc(IN);
		$s = $s . $c;
	}while( $s =~ /[[]/ && $c !~ /[A-Za-z~]/ );
	$s;
}
	  
sub insert_char {
	local($c,*len,*ix,*hist) =@_;
	local($clen);
	$clen = length $c;    
	if( $init_in ) {
		$len = $ix = $clen; # new hist - clear old one
		$hist[$#hist] = $c;
	}else{
		substr($hist[$#hist],$ix,0) = $c;   #insert char
		$len += $clen;
		$ix += $clen;
	}
}
sub termsize {
	my($row, $col,$s);
	$s =stty ("-a");
	($row,$col) = ($s =~ /(\d+)\s+rows[,\s]+(\d+)\s+columns/ );
}

sub stty    {
	local($arg) = @_;
	if( -f "/usr/5bin/stty" ) {
		`/usr/5bin/stty $arg <$tty`;
	}elsif( -f "/usr/bin/stty" ) {
		`/usr/bin/stty $arg >$tty`;
	}else {
		`/bin/stty $arg >$tty`;
	}
}

sub add_hist {
  # add input into history file
	local($type,*cmd) = @_;	#not my
	my( $t )= sprintf("%s",$type);
	my($h) = $cmd[$#cmd];
	return if !defined $File;
	if( $#cmd ==0 || $h ne $cmd[$#cmd-1] )        {
		$h =~ s/([\"@\$])/\\$1/g;
		$t =~ s/^\*//;
		push(@Histall, "push  (\@$t, \"$h\");\n" );
		@Histall = splice( @Histall, -$HIST_SIZE, $HIST_SIZE ); # take last HIST_SIZE commands
		if( open( FILE, ">$File" ) ){
			print FILE @Histall;
			print FILE "1;\n";
			close(FILE);
		}
	}else {
		$#cmd--;
	}
}

sub main::input {
	# input line without output \n
	# arg0 - prompt
	# arg1 - input stack
	# arg2 - append input to command if 1
	# arg3 - # of column offset
	local($prompt,*hist,$app,$off) = @_;
	local($len,$ix);
	local($c,$ix0,$init_in,$isp,$plen,$offset,$s);
	my($tmp, $isp_cur,$x,$xc,$y_ix,$y_len,$wd,$ht,$col);
	my($p_save);
	if( !defined $off ) {
		$off = 0;
	}
	$plen = length($prompt);
	if( ! defined @hist ) { *hist = *Hist; }
	$isp = ++$#hist ;
	&stty(" -echo cbreak");
	($ht,$wd) = &termsize();
	fcntl(IN,4,0);				#   no real time input
	$offset = ($TERM_RIGHT) x $off;
	$y_ix0 = $y_len = 0; 
	$mode = 'n'; #normal mode
	IN_STACK: while(1){
		  $init_in = 1-$app;
		  ($hist[$#hist] = $hist[$isp]) =~ s/\n//;
		  $ix = $ix0 = length($hist[$#hist]);
		  do  {
			  $len = length($hist[$#hist]);
			  #move cursor to bol on screen
			  print OUT $TERM_UP x ($y_ix0) , "\r$TERM_EEOL"; 
			  # erase prev printout except top line
			  print OUT "\n\r$TERM_EEOL" x $y_len;  
			  print OUT $TERM_UP x $y_len, "\r";
			  $y_len = int (($plen+$len-1+$off) / $wd );
			  $y_ix = int (($plen+$ix+$off) / $wd );
			  $ix0 = $ix; #previous ix value
			  $y_ix0 = int (($plen+$ix0+$off) / $wd );
			  $x = ($plen+$ix+$off) % $wd;

			  print OUT "$offset$prompt$hist[$#hist]";

			  if( $mode eq 's' ) {
				  #move cursor to search string
				  $col = $TERM_RIGHT x (length($prompt)-2 );
			  }else{
				  #normal mode - move cursor back to $ix
				  $col = ($TERM_RIGHT) x $x; 
			  }				  
			  print OUT $TERM_UP x ($y_len), "\n" x $y_ix , "\r$col";

 			  $c = getc(IN);
			  if( $c eq "$ESC" )  {  
				  $c .= &escape; 
			  }			  

			  if( $Func{$c} =~ /ign-mode/ ) {
				  # ignore mode and execute command
				  if( $Func{$c} =~ /boh/ ) {
					  $isp = 0;
				  }elsif( $Func{$c} =~ /eoh/ ) {
					  $isp = $#hist;
				  }
				  next IN_STACK;
			  }elsif( $mode eq 's' ) {
				  if($c eq "\n"){
					  $prompt = $p_save; 
					  $mode = 'n';
					  last IN_STACK;
				  }
				  $isp_cur = $isp;
				  if( $Func{$c} eq 'search-rev' ) {
					  #search furthur
					  while(1) {
						  if( --$isp<0 ) { 
							  print OUT "\a"; # couldn't find one
							  $isp = $isp_cur;
							  last;
						  }
						  last if( index($hist[$isp],$s) >=0);
					  }
				  }elsif( $Func{$c} eq 'bs' ) {
					  $s =~ s/.$//;
				  }elsif( ord($c) < 32 ) {
					  #non-printable char, get back to normal mode
					  print OUT "\a"; 
					  $prompt = $p_save; 
					  $mode = 'n';
					  next IN_STACK;
				  }else{
					  $s .= $c;
					  while(1) {
						  last if (index($hist[$isp],$s) >=0);
						  if( --$isp<0 ) { 
							  print OUT "\a";   #couldn't find one
							  chop($s); 
							  $isp = $isp_cur;
							  last;
						  }
					  }
				  }
				  $prompt = "(search)'$s':";
				  next IN_STACK;
			  }elsif( $c eq $KEY_EOF && $len==0 ) {   
				  return '';	# eof return null
			  }elsif( $c eq "\n" ) {
				  last IN_STACK;
			  }elsif( $Func{$c} eq 'quote' ) {
				  $c = getc(IN);
				  if( $c eq "$ESC" )  {  $c = &escape; }
				  &insert_char($c,*len,*ix,*hist);
			  }elsif( $Func{$c} eq 'prev-char' ) {
				  if ($ix>0) {$ix--;}
			  }elsif( $Func{$c} eq 'next-char' ) {
				  if ($ix<$len)  {$ix++;}
			  }elsif( $Func{$c} eq 'cancel' ) {
				  $hist[$#hist] = "";
				  $len = 0;
				  last IN_STACK;
			  }elsif( $Func{$c} eq 'next-word' ) {
				  $hist[$#hist] =~ /^(.{$ix}\S*(\s+|$))/;
				  $ix = length($1);
			  }elsif( $Func{$c} eq 'prev-word' ) {
				  $tmp = substr($hist[$#hist],0,$ix);
				  $tmp =~ s/(^|\S+)\s*$//;
				  $ix = length($tmp);
			  }elsif( $Func{$c} eq 'del-word' ) {
				  $tmp = substr($hist[$#hist],0,$ix);
				  $tmp =~ s/(^|\S+)\s*$//;
				  $tmp = length $tmp;
				  substr($hist[$#hist],$tmp,$ix-$tmp) = "";
				  $ix = $tmp;
			  }elsif( $Func{$c} eq 'prev-line' ) {
				  if($isp>0) { 
					  $isp--;
				  }else {
					  $isp = 0;
					  print "\a";
				  }
				  next IN_STACK;
			  }elsif( $Func{$c} eq 'next-line' ) {
				  if($isp<$#hist) {
					  $isp++;
				  }else {
					  $isp = $#hist-1;
					  print "\a";
				  }
				  next IN_STACK;
			  }elsif( $Func{$c} eq 'bol' ) {
				  $ix = 0; 
			  }elsif( $Func{$c} eq 'eol' ) {
				  $ix = $len; 
			  }elsif( $Func{$c} eq 'bs' )  { 
				  if( $len && $ix ) {
					  $len--;
					  $ix--;	# mv left
					  substr($hist[$#hist],$ix,1) = "";   # del char
				  }
			  }elsif( $Func{$c} eq 'del-char' ) {
				  if( $len > $ix ) {
					  $len--;
					  substr($hist[$#hist],$ix,1) = "";   # del char
				  }
			  }elsif( $Func{$c} eq 'clr-eol' ) { 
				  $len = $ix;  
				  substr($hist[$#hist],$ix) = "";      
			  }elsif( $Func{$c} eq 'del-line' ) { 
				  $len = $ix = 0;
				  $hist[$#hist] = "";
			  }elsif( $Func{$c} eq 'search-rev' ) {   
				  $s = '';
				  $mode = 's'; #search mode
				  $p_save = $prompt;
				  $prompt = "(search)'$s':";
				  $hist[$#hist] = $hist[$isp];
			  }elsif( length $c > 1 ) { 
				  if( $Ksel & 4 ) { # return escape code
					  &insert_char($c,*len,*ix,*hist);
					  last IN_STACK;
				  }
				  print "\a"; 			# undefined esc char
			  }elsif( ord ($c) < 32 ) { #undefined control char
				  print "\a";
			  }else{ 
				  &insert_char($c,*len,*ix,*hist); 
			  }
		      $init_in = 0;
		  } while(1);
	      last;
	  }
	&stty ("echo -cbreak");
	if( defined $main::stty_setup ) { &stty($main::stty_setup);}
	print OUT "\n";
	if( $#hist>0 && $hist[$#hist] eq $hist[$#hist-1] ) { pop(@hist); }   # if it is the same, delete
	else{ &add_hist( *hist, *hist ); }
	$hist[$#hist]."\n";
}




