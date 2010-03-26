use strict;
use warnings;
use Time::HiRes qw(gettimeofday);

use constant {
    TESTCOUNT => 20,
    REMOVETOP => 2,
    REMOVEBOTTOM => 8,
};

my @commands = (
                "experiment/mmap"
                );

my @table;
foreach my $command (@commands){
    system("sync ; sync");
    my @result;
    for(my $i = 0; $i < TESTCOUNT; $i++){
        my $stime = gettimeofday();
        system("$command >/dev/null");
        $stime = gettimeofday() - $stime;
        push(@result, $stime);
    }
    @result = sort { $a <=> $b } @result;
    for(my $i = 0; $i < REMOVETOP; $i++){
        shift(@result);
    }
    for(my $i = 0; $i < REMOVEBOTTOM; $i++){
        pop(@result);
    }
    my $sum = 0;
    foreach my $result (@result){
        $sum += $result;
    }
    my $avg = $sum / scalar(@result);
    push(@table, [$command, $avg]);
}

printf("\n\nRESULT\n");
foreach my $row (@table){
    printf("%s\t%0.5f\n", $$row[0], $$row[1]);
}
printf("\n");
