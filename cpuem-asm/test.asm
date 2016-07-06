testfor7
{
	byte 7 breq.is7
		int 0 br.calltest
	is7:
		int 0 ~

	calltest:
		testfn(4)

	halt
}

testfn
{
	halt
}

entry
{
	byte 7
	testfor7(1)
	double -3.14e0
	halt
}