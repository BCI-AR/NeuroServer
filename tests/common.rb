#
# Test suite for NeuroServer software by Rudi Cilibrasi (cilibrar@ofb.net)
#
# Set the environment variable NSDHOST to point to your nsd server
#
# At this point this test suite tests only nsd
#

require 'socket'

@verbose = false
@con = nil
@history = [ ]
@testname = nil

EDFHDR = "0                                                                                                                                                                       25.01.0417.40.51768     NEUROSD                                     -1      1       2   Elec 00         Elec 01         Active Electrode                                                                Active Electrode                                                                uV      uV           0       0   1023    1023        0       0   1023    1023   HP: DC; LP:  49 Hz                                                              HP: DC; LP:  49 Hz                                                              2048    2048    Reserved                        Reserved                        "

def clearHistory()
	@history = [ ]
end

def beginTest(str)
	puts "Beginning test #{str}"
	@testname = str
	clearHistory()
end

def finishedTest()
	puts format("PASSED %20s %20s %10s", @history[-1][0], @history[-1][1], @testname)
end

def connectToServer()
	hostname = ENV['NSDHOST']
	port = 8336
	hostname = 'localhost' unless hostname
	@con = TCPSocket.new(hostname, port)
end

def failedTest()
	puts "Test #{@testname} FAILED"
	@history.each { |h|
		puts h.join(',')
	}
	exit(1)
end

def getResponse()
	result = getLine()
	if result
		result.chomp!
		@history[-1] << result
	end
	result
end

def getLine()
	result = @con.gets
	result.chomp! if result
	puts "got response line:<#{result}>" if (@verbose)
	result
end

def shouldBe(str)
	got = getLine
	if got != str
		puts "ERROR got <#{got}> but should be <#{str}>"
		failedTest
	end
end


def getBadRequest()
	result = getResponse()
	failedTest unless result =~ /^400/
end

def getOK()
	result = getResponse()
	failedTest unless result =~ /^200/
end

def sendCommand(str)
	@con.puts str
	@history << [str]
	print format("%-20s", str) if (@verbose)
end

def doTest01()
	beginTest("01-close")
	connectToServer()
	sendCommand("close")
	getOK()
	finishedTest()
end

def doTest02()
	beginTest("02-role")
	connectToServer()
	sendCommand("role")
	getOK()
	shouldBe('Unknown')
	sendCommand("display")
	getOK()
	sendCommand("role")
	getOK()
	shouldBe('Display')
	sendCommand("close")
	getOK()
	connectToServer()
	sendCommand("eeg")
	getOK()
	sendCommand("role")
	getOK()
	shouldBe('EEG')
	sendCommand("close")
	getOK()
	connectToServer()
	sendCommand("control")
	getOK()
	sendCommand("role")
	getOK()
	shouldBe('Controller')
	sendCommand("close")
	getOK()
	finishedTest()
end

def doTest03()
	beginTest("03-status")
	connectToServer()
	sendCommand("display")
	getOK()
	sendCommand("status")
	getOK()
	shouldBe("1 clients connected")
	shouldBe("0:Display")
	sendCommand("close")
	getOK()
	finishedTest()
end

def doTest04()
	beginTest("04-eeg!")
	connectToServer()
	sendCommand("eeg")
	getOK()
	sendCommand("setheader #{EDFHDR}")
	getOK()
	sendCommand("! 0 2 15 41")
	getOK()
	sendCommand("close")
	getOK()
	finishedTest()
end

def doTest05()
	beginTest("05-watch")
	connectToServer()
	eegCon = @con
	sendCommand("eeg")
	getOK()
	connectToServer()
	displayCon = @con
	sendCommand("display")
	getOK()
	@con = eegCon
	sendCommand("setheader #{EDFHDR}")
	getOK()
	@con = displayCon
	sendCommand("watch 0")
	getOK()
	@con = eegCon
	sendCommand("! 0 2 15 41")
	getOK()
	sendCommand("close")
	getOK()
	@con = displayCon
	shouldBe("! 0 0 2 15 41")
	sendCommand("close")
# TODO: Figure out why this response is not coming in
#	getOK()
	finishedTest()
end

def doTests()
	doTest01()
	doTest02()
	doTest03()
	doTest04()
	doTest05()
end

doTests()
