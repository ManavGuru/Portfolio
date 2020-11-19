if {$argc !=2 } {
	puts"usage ns ns2.tcl TCP_FLAVOR CASE_NO"
}

set tcp_flv [lindex $argv 0]
set case_no [lindex $argv 1]

global e2e_delay 
global flavor

set e2e_delay 0

if {$case_no == 1} {
	set e2e_delay "12.5ms"
} elseif {$case_no == 2} {
	set e2e_delay "20ms"
} elseif {$case_no == 3} {
	set e2e_delay "27.5ms"
} else {
	puts "Invalid case_no! case_no is between 1-3, you entered $case_no"
	exit
}

if {$tcp_flv == "SACK"} {
	set flavor "Sack1"
} elseif {$tcp_flv == "VEGAS"} { 
	set flavor "Vegas"
} else {
	puts "Invalid TCP Flavor, use either SACK or VEGAS. You used $tcp_flv."
	exit
}

set ns [new Simulator]
set file "output_$flavor$case_no"

set flow1 [open out1.tr w]
set flow2 [open out2.tr w]

set tracefile [open out.tr w]
$ns trace-all $tracefile 

set nf [open out.nam w]
$ns namtrace-all $nf

# creating the nodes
set src1 [$ns node]
$src1 color Blue
$src1 label source1

set src2 [$ns node]
$src2 color Red
$src2 label source2

set r1   [$ns node]
$r1 shape square
$r1 color Green
$r1 label router1

set r2   [$ns node]
$r2 shape square
$r2 color Green
$r2 label router2

set rcv1 [$ns node]
$rcv1 color Blue
$rcv1 label receiver1

set rcv2 [$ns node]
$rcv2 color Red
$rcv2 label receiver2

$ns color 1 Red 
$ns color 2 Blue

# defining variable for throughput calc
set throughput1 0
set throughput2 0 
set count 0

# creating the transports 
set tcp [new Agent/TCP/$flavor]
set tcp2 [new Agent/TCP/$flavor]
# making connections
$ns attach-agent $src1 $tcp
$ns attach-agent $src2 $tcp2

$tcp set class_ 1
$tcp2 set class_ 2

# creating the links 
$ns duplex-link $src1 $r1 10Mb 5ms DropTail
$ns duplex-link $src2 $r1 10Mb 5ms DropTail

$ns duplex-link $r1 $r2 1Mb 5ms DropTail

$ns duplex-link $rcv1 $r2 10Mb $e2e_delay DropTail
$ns duplex-link $rcv2 $r2 10Mb $e2e_delay DropTail


# Creating topology in graph
$ns duplex-link-op $src1 $r1 orient right-down
$ns duplex-link-op $src2 $r1 orient right-up
$ns duplex-link-op $r1 $r2 orient right
$ns duplex-link-op $r2 $rcv1 orient right-up
$ns duplex-link-op $r2 $rcv2 orient right-down

set ftp [new Application/FTP]
set ftp2 [new Application/FTP]

$ftp attach-agent $tcp 
$ftp2 attach-agent $tcp2

# creating the sinks for tcp
set tcpsink [new Agent/TCPSink]
set tcpsink2 [new Agent/TCPSink]
$ns attach-agent $rcv1 $tcpsink 
$ns attach-agent $rcv2 $tcpsink2

$ns connect $tcp $tcpsink
$ns connect $tcp2 $tcpsink2 

proc finish {} {
	global ns nf tracefile nf file throughput1 throughput2 count
	$ns flush-trace 

	puts "Avg throughput for Source 1 =[expr $throughput1/$count] Mbps\n"
	puts "Avg throughput for Source 2 =[expr $throughput2/$count] Mbps\n"
	close $tracefile
	close $nf 
	exec nam out.nam & 
	exit 0
}

proc calculate {} {
	global tcpsink tcpsink2 flow1 flow2 throughput1 throughput2 count
	
	set ns [Simulator instance]

	set time 0.5

	set bandwidth1 [$tcpsink set bytes_]
	set bandwidth2 [$tcpsink2 set bytes_]

	set current_time [$ns now]

	puts $flow1 "$current_time [expr $bandwidth1/$time*8/1000000]"
	puts $flow2 "$current_time [expr $bandwidth2/$time*8/1000000]"
	set throughput1 [expr $throughput1 + $bandwidth1/$time*8/1000000]
	set throughput2 [expr $throughput2 + $bandwidth2/$time*8/1000000]
	set count [expr $count + 1]

	$tcpsink set bytes_ 0 
	$tcpsink2 set bytes_ 0

	$ns at [expr $time + $current_time] "calculate"
}

$ns at 0 "calculate"
$ns at 0 "$ftp start"
$ns at 0 "$ftp2 start"
$ns at 400 "$ftp stop"
$ns at 400 "$ftp2 stop"
$ns at 400 "finish"
$ns color 1 Blue 
$ns color 2 Red 
set tf1 [open "$file-src1.tr" w]
$ns trace-queue $src1 $r1 $tf1
set tf2 [open "$file-src2.tr" w]
$ns trace-queue $src2 $r1 $tf2

$ns run