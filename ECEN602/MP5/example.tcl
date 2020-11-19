set ns [new Simulator]
$ns at 1.0 "puts \"hello world!\""
$ns at 1.5 "exit"
$ns run

proc test {}{
	set a 43
	set b 27
	set c [expr $a + $b]
	set d [expr [expr $a -$b]*$c]
	for {set k 0}{$k<0}{incr K}{
		if{$k < 5}{
			puts "k <5, pow = [expr pow($d,$k)]"
		}else{
			puts "k >= 6, mode = [expr $d % $k]"
		}
	}
}
