#!/usr/bin/perl -w

if ($#ARGV < 1) {
  print "usage: $0 <source_file> <num_partition>\n";
  exit (0);
}

my $source_file= $ARGV[0];
my $num_part= $ARGV[1];

my $vertex_num= `head -n 1 $source_file`; chomp($vertex_num);

my @edge_cnt;
my @output_file;

for (my $i=1; $i<=$num_part; $i++) {
   $edge_cnt[$i]= 0;
}

open IN, "$source_file" or die "can't open $source_file!\n";
while (<IN>) { # $_
    if (/(\d+) (\d+).*/) {
        $file_index = $1 % $num_part + 1;
        $edge_cnt[$file_index]++;
    }
}
close IN;

my $file1_num = $vertex_num % $num_part;
my $total_vertex = $vertex_num / $num_part + 1;
for(my $i = 1; $i <= $file1_num; ++$i) {
    open $output_file[$i], "> ${source_file}_${num_part}w_$i" or die "can't open ${source_file}_${num_part}w_$i!\n";
    printf {$output_file[$i]} "%d\n", $total_vertex;
    printf {$output_file[$i]} "%d\n", $edge_cnt[$i];
}

$total_vertex = $vertex_num / $num_part;
for(my $i = $file1_num + 1; $i <= $num_part; ++$i) {
    open $output_file[$i], "> ${source_file}_${num_part}w_$i" or die "can't open ${source_file}_${num_part}w_$i!\n";
    printf {$output_file[$i]} "%d\n", $total_vertex;
    printf {$output_file[$i]} "%d\n", $edge_cnt[$i];
}

open IN, "$source_file" or die "can't open $source_file!\n";
while (<IN>) { # $_
    if (/(\d+) (\d+).*/) {
        $file_index = $1 % $num_part + 1;
        print {$output_file[$file_index]} "$1 $2\n";
    }
}
close IN;

for($i = 1; $i <= $num_part; ++$i) {
    close $output_file[$i];
}
