import time
import random
import sys


def test(n):
	for i in range(n):
		a = i * i / 3.14159

def main(dict):
	retDict = {}
	print(str(dict["dev-testStr"]), file=sys.stderr)
	start = time.time()
	test(20000000)
	end = time.time()
	retDict["usingTime"] = end - start
	retDict["inputStr"] = str(dict["dev-testStr"])
	return retDict

# ret = main({})
# print(ret["usingTime"])