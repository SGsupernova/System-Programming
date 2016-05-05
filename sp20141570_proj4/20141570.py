#!/usr/bin/python2
from bs4 import BeautifulSoup
import requests

req = requests.get("http://cspro.sogang.ac.kr/~gr120160213/index.html")
soup = BeautifulSoup(req.content, "html.parser")
results = soup.find_all('a')


name = "http://cspro.sogang.ac.kr/~gr120160213/"

name_len = len(name)

# make file
urlFile = open("URL.txt", "w")

visitedUrlList = []

for link in results:
	urlList = [ link ]

	###### BFS #######
	while urlList :
		urlLink = urlList.pop()

		# get hyperLink
		aString = urlLink.get('href')

		# remove that link calls itself
		if (aString.find("#") > -1) or (aString.find("?") > -1) :
			continue

		# there is no href
		if not aString :
			continue

		# make URL
		if (aString.find(name) > -1) :
			urlString = aString
		else :
			if aString.find('/') == 0 :
				urlString = name + (aString + 1)
			else :
				urlString = name + aString
		
		flag = 0
		# duplicate check
		for i in visitedUrlList :
			# duplicate url
			if i == urlString :
				flag = 1
				break;

		if flag == 1 :
			continue;

		req = requests.get(urlString)
	
		if not req.ok :
			continue

		# make file name
		end = urlString.find(".html")
		urlFileName = urlString[name_len:end] + ".txt"
		

		outputFile = open(urlFileName, "w")
		outputFile.write(req.text)

		# check visiting page
		visitedUrlList.append(urlString)

		urlFile.write(urlString + "\n")

		soup = BeautifulSoup(req.content, "html.parser")
		results = soup.find_all('a')

		# cancatenation (add link)
		urlList = urlList + results

		
	#################

#
#	urlFile.write(urlString)
#	
#	print "string : ", string


urlFile.close()
