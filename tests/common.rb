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

def clearHistory()
	@history = [ ]
end

def beginTest(str)
	puts "Beginning test #{str}"
	@testname = str
end

def finishedTest()
	puts format("PASSED %20s %20s %10s", @history[-1][0], @history[-1][1], @testname)
end

def connectToServer()
	hostname = ENV['NSDHOST']
	port = 8336
	hostname = 'localhost' unless hostname
	@con = TCPSocket.new(hostname, port)
	clearHistory()
end

def failedTest()
	puts "Test #{@testname} FAILED"
	@history.each { |h|
		puts format("%30s %30s", h[0], h[1])
	}
	exit(1)
end

def getResponse()
	result = getLine()
	result.chomp!
	@history[-1] << result
	result
end

def getLine()
	result = @con.gets
	result.chomp!
	puts result if (@verbose)
	result
end

def shouldBe(str)
	got = getLine
	if got != str
		puts "got <#{got}> but should be <#{str}>"
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

def doTests()
	doTest01()
	doTest02()
end

doTests()
