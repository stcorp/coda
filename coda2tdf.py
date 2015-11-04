#!/usr/bin/python


import getopt
import sys
import os
from os.path import exists

import xml.dom as Dom
from xml.dom.minidom import getDOMImplementation
from xml.dom.minidom import parse
from xml.dom import DOMException


help = """
Usage: ./coda2tdf.py [options]
options:
       -c 'codadef path' eg: "/*/*/definitions"
       -t 'template path' eg: "/*/*/testdeftemplate.xml"
       -p 'output file' eg: "/*/AE_TESTDEF.xml"
          wil be overwritten if exists!
       -r restrict the test definition file to the product 
          types in the supplied file, optional.
       -h return this message

The prefix -c,-t,-p isn't needed, when ommited, 
pos1 = codadef path, pos2 = template path,
pos3 = output file.

"""

codaDefPath  = None
indexPath    = None
testsPath    = None
templatePath = None
outputPath   = None
restrictPath = None
File_Name    = None


outputHandle = None

templateDoc  = None
testDoc      = None
tempDoc      = None

loNamedTests          = None
loNamedCrossFileTests = None
loTestDefinitions     = None

productList = []
restrictList = []
restrictActive = False

namedTestList = []
namedCrossFileTestList = []
NSS = {u'cd': u"http://www.stcorp.nl/coda/definition/2008/07",
       u'ct': u"http://www.stcorp.nl/coda/test/2008/10"}


def getArgs():
    """
       Function to retrieve the parameters supplied by command line.
    """
    global opts, args
    global codaDefPath, templatePath, outputPath
    global restrictPath, restrictActive

    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:t:p:r:h")
    except:
        print help
        sys.exit()

    for o, v in opts:
        if o == "-h":
            print help
            sys.exit()
        elif o == "-c":
            codaDefPath = v
        elif o == "-t":
            templatePath = v
        elif o == "-p":
            outputPath = v
        elif o == '-r':
            restrictPath = v
            restrictActive = True


    if codaDefPath == None or templatePath == None or outputPath == None :
        if len(args)>=3:
            codaDefPath = args[0]
            templatePath = args[1]
            outputPath = args[2]
            if len(args)==4:
                restrictPath = args[3]
                restrictActive = True
        else:
            print 'not enough parameters supplied!'
            print help
            sys.exit()


def setPathVars():
    """
       Function to set certain path variables.
    """
    global indexPath, testsPath, File_Name

    indexPath = codaDefPath + '/index.xml'
    testsPath = codaDefPath + '/tests.xml'
    File_Name = os.path.basename(outputPath)


def testFiles():
    """
       Function which tests if given parameters are correct.
    """
    global outputHandle, restrictList
    error = False

    if False == exists(indexPath):
        print "File doesn't exists, index.xml on: ", indexPath, ' !'
        error = True
        
    if False == exists(templatePath):
        print "File doesn't exists, template on: ", templatePath, ' !'
        error = True

    try:
        outputHandle = open(outputPath, "w")
    except:
        print "Couldn't open output file on: ", outputPath, '!'
        error = True

    if False == exists(testsPath):
        print "File doesn't exists, test.xml on: ", namedTestsPath, ' !'
        error = True

    if restrictActive:
        if exists(restrictPath):
            restrictHandle = open(restrictPath)
            restrictList = restrictHandle.readlines()
            restrictHandle.close()
        else:
            print "Couldn't open file with restricted product types, ", restrictPath, '!'
            error = True
    if error:
        sys.exit()


def getFile(fileName):
    """
       Try to parse a file and return the results.
    """
    error = False
    document = None
    if True == exists(fileName):
        try:
            document = parse(fileName)
        except Exception,e:
            print 'File, ',fileName,', import error: ', e
            sys.exit()
    else:
        print "Function getFile, following file didn't exists: ", fileName
    
    return document


def lookUpId(id):
    """
       Try to retrieve the file associated with the given id, return None when not found. Currently two locations 
       supported, types and producs.
    """
    global codaDefPath
    
    loc1 = codaDefPath + '/products/' + id + '.xml'
    loc2 = codaDefPath + '/types/' + id + '.xml'

    if True == exists(loc1):
        return loc1
    elif True == exists(loc2):
        return loc2
    else:
        print "Function lookUpId, couldn't find id: ", id , 'on:', loc1, loc2
        return None


def getPathName(node):
    """ 
       Retrieve the path of the given node by summation of the Field name attributes from parent, parent parent etc
       until the root node is reached.
    """
    str = str2 = ''

    if node.namespaceURI == NSS['cd']:
        if node.nodeName == 'cd:Field':
            str = '/' + node.getAttribute('name')
    if node.parentNode.nodeType == node.nodeType:
        if node.parentNode.nodeName == 'cd:Array':
            str = '[]'
        str2 = getPathName(node.parentNode)

    if len(str) == 0:
        return str2
    else:
        if len(str2) == 0:
            if str[0] != '/':
                return '/' + str
            else:
                return str
        return str2 + str


def collectId(fileId):
    """
       Retrieve the document associated with the id. Check if it contains also id's to retrieve, merge them and return 
    """
    document = None
    file = lookUpId(fileId)

    if None == file:
        print 'File not found', fileId
        return None

    document = getFile(file)

    idCollect = document.getElementsByTagNameNS(NSS['cd'], 'NamedType')
    for id in idCollect:
        getId = id.getAttribute(u'id')
        doc = collectId(getId)
        if None != doc:
            for nodes in doc.childNodes:
                id.appendChild(nodes)

    return document


def getProductList():
    """
       Parse the index file and retrieve the contained products.
    """
    path = codaDefPath + '/index.xml'
    doc = getFile(path)
    idCollect = doc.getElementsByTagNameNS(NSS['cd'], 'ProductDefinition')
    for id in idCollect:
        if id.namespaceURI != NSS['ct']:
            productType = id.parentNode.getAttribute(u'name')
            productId = id.getAttribute(u'id')
            productVersion = id.getAttribute(u'version')
            subList = [productType, productId, productVersion]
            allowed = True
            if restrictActive:
                allowed = False
                for n in restrictList:
                    s = n.replace('\n', '')
                    if s == productType:
                        allowed = True
            if allowed:
                productList.append(subList)


def getAndSetupDocs():
    """
       Get the documents containing the named tests and the template.
    """
    global NSS, loNamedTests, loNamedCrossFileTests, loTestDefinitions, testDoc, templateDoc

    testDoc = parse(testsPath)
    templateDoc = parse(templatePath)
    NSS['td'] = templateDoc.documentElement.namespaceURI

    loNamedTests = templateDoc.getElementsByTagName('List_of_Named_Tests')[0]
    loNamedCrossFileTests = templateDoc.getElementsByTagName('List_of_Named_Cross_File_Tests')[0]
    loTestDefinitions = templateDoc.getElementsByTagName('List_of_Test_Definitions')[0]
    

def appendNamedTest(name):
    """
       Check if the List_of_Named_Tests contains the refered test, otherwise, append the test.
    """
    global loNamedTests, namedTestList

    for entry in namedTestList:
        if entry[0] == name:
            return entry[1]

    dictionairy = testDoc.documentElement.getElementsByTagNameNS(NSS['ct'],'Test')

    found = False
    for test in dictionairy:
        if test.getAttribute('name') == name:
            found = True
            copyNode = test

    if found == False:
        print 'NamedTest: ', name, 'not found in named test collection, abort'
        sys.exit()

    doc = loNamedTests.ownerDocument

    namedTest = doc.createElement('Named_Test')

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(name))
    namedTest.appendChild(element)
    
    element = doc.createElement('Description')
    element.appendChild(doc.createTextNode(copyNode.getAttribute('description')))
    namedTest.appendChild(element)
    
    element = doc.createElement('Criticality')
    element.appendChild(doc.createTextNode(copyNode.getAttribute('criticality')))
    namedTest.appendChild(element)

    element = doc.createElement('Test_Expression')
    element.appendChild(doc.createCDATASection(copyNode.firstChild.nodeValue))
    namedTest.appendChild(element)

    loNamedTests.appendChild(namedTest)
    
    path = copyNode.getAttribute('path')
    namedTestList.append([name, path])

    return path

def appendNamedCrossFileTest(name):
    """
       Check if the List_of_Named_Cross_File_Tests contains the refered test, otherwise, append the test.
    """
    global loNamedCrossFileTests, namedCrossFileTestList

    for entry in namedCrossFileTestList:
        if entry == name:
            return

    dictionairy = testDoc.documentElement.getElementsByTagNameNS(NSS['ct'],'CrossFileTest')

    found = False
    for test in dictionairy:
        if test.getAttribute('name') == name:
            found = True
            copyNode = test

    if found == False:
        print 'NamedCrossFileTest: ', name, 'not found in named cross file test collection, abort'
        sys.exit()

    doc = loNamedCrossFileTests.ownerDocument

    namedCrossFileTest = doc.createElement('Named_Cross_File_Test')

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(name))
    namedCrossFileTest.appendChild(element)
    
    element = doc.createElement('Description')
    element.appendChild(doc.createTextNode(copyNode.getAttribute('description')))
    namedCrossFileTest.appendChild(element)
    
    element = doc.createElement('Criticality')
    element.appendChild(doc.createTextNode(copyNode.getAttribute('criticality')))
    namedCrossFileTest.appendChild(element)

    expressionNode = copyNode.getElementsByTagNameNS(NSS['ct'],'ValueExpressionDbl')[0]
    element = doc.createElement('Value_Expression_Dbl')
    element.appendChild(doc.createCDATASection(expressionNode.firstChild.nodeValue))
    namedCrossFileTest.appendChild(element)

    expressionNode = copyNode.getElementsByTagNameNS(NSS['ct'],'ValueExpressionHdr')[0]
    element = doc.createElement('Value_Expression_Hdr')
    element.appendChild(doc.createCDATASection(expressionNode.firstChild.nodeValue))
    namedCrossFileTest.appendChild(element)

    loNamedCrossFileTests.appendChild(namedCrossFileTest)
    
    namedCrossFileTestList.append(name)


def setupTestDefinition(productType, formatVersion = '0'):
    """
       Create a product specific test definition.
       Append it to the list and return the places to append the (named) tests.
    """
    global loTestDefinitions

    doc = loTestDefinitions.ownerDocument

    testDefinition = doc.createElement('Test_Definition')

    element = doc.createElement('Product_Type')
    element.appendChild(doc.createTextNode(productType))
    testDefinition.appendChild(element)

    element = doc.createElement('Format_Version')
    element.appendChild(doc.createTextNode(formatVersion))
    testDefinition.appendChild(element)

    loTests = doc.createElement('List_of_Tests')
    testDefinition.appendChild(loTests)
    
    loNamedTestReferences = doc.createElement('List_of_Named_Test_References')
    testDefinition.appendChild(loNamedTestReferences)

    loCrossFileTests = doc.createElement('List_of_Cross_File_Tests')
    testDefinition.appendChild(loCrossFileTests)

    loNamedCrossFileTestReferences = doc.createElement('List_of_Named_Cross_File_Test_References')
    testDefinition.appendChild(loNamedCrossFileTestReferences)

    loTestDefinitions.appendChild(testDefinition)

    return loTests, loNamedTestReferences, loCrossFileTests, loNamedCrossFileTestReferences


def handleNamedTestReference(testNode, loNamedTestReferences):
    """
       Handle this node which refers to a named test.
    """
    doc = loNamedTestReferences.ownerDocument

    testName = testNode.getAttribute('id')
    pathTest = testNode.getAttribute('path')
    pathRef = appendNamedTest(testName)
    nodePath = getPathName(testNode.parentNode)
    path = ''

    if len(pathTest):
        if (pathTest[0] == '[') or (pathTest[0] == '@'):
            path = nodePath + pathTest
        else:
            path = nodePath + '/' + pathTest
    else:
        if pathRef != None and len(pathRef):
            if (pathRef[0] == '[') or (pathRef[0] == '@'):
                path = nodePath + pathRef
            else:
                path = nodePath + '/' + pathRef
        else:
            path = nodePath

    if path == '':
        path = '/'

    namedTestReference = doc.createElement('Named_Test_Reference')

    element = doc.createElement('Path')
    element.appendChild(doc.createTextNode(path))
    namedTestReference.appendChild(element)

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(testName))
    namedTestReference.appendChild(element)

    loNamedTestReferences.appendChild(namedTestReference)
    

def handleTest(testNode, loTests):
    """
       Handle this node which refers to a test.
    """
    doc = loTests.ownerDocument

    testName = testNode.getAttribute('name')
    pathTest = testNode.getAttribute('path')
    nodePath = getPathName(testNode.parentNode)
    path = ''

    if len(pathTest):
        if (pathTest[0] == '[') or (pathTest[0] == '@'):
            path = nodePath + pathTest
        else:
            path = nodePath + '/' + pathTest
    else:
        path = nodePath

    if path == '':
        path = '/'

    test = doc.createElement('Test')

    element = doc.createElement('Path')
    element.appendChild(doc.createTextNode(path))
    test.appendChild(element)

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(testName))
    test.appendChild(element)
 
    element = doc.createElement('Description')
    element.appendChild(doc.createTextNode(testNode.getAttribute('description')))
    test.appendChild(element)

    element = doc.createElement('Criticality')
    element.appendChild(doc.createTextNode(testNode.getAttribute('criticality')))
    test.appendChild(element)

    element = doc.createElement('Test_Expression')
    element.appendChild(doc.createCDATASection(testNode.firstChild.nodeValue))
    test.appendChild(element)

    loTests.appendChild(test)


def handleNamedCrossFileTestReference(crossFileTestNode, loNamedCrossFileTestReferences):
    """
       Handle this node which refers to a named cross file test.
    """
    doc = loNamedCrossFileTestReferences.ownerDocument

    testName = crossFileTestNode.getAttribute('id')
    appendNamedCrossFileTest(testName)

    namedCrossFileTestReference = doc.createElement('Named_Cross_File_Test_Reference')

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(testName))
    namedCrossFileTestReference.appendChild(element)

    loNamedCrossFileTestReferences.appendChild(namedCrossFileTestReference)
    

def handleCrossFileTest(crossFileTestNode, loCrossFileTests):
    """
       Handle this node which refers to a Cross File Test node
    """
    doc = loCrossFileTests.ownerDocument

    crossFileTest = doc.createElement('Cross_File_Test')

    element = doc.createElement('Name')
    element.appendChild(doc.createTextNode(crossFileTestNode.getAttribute('name')))
    crossFileTest.appendChild(element)
 
    element = doc.createElement('Description')
    element.appendChild(doc.createTextNode(crossFileTestNode.getAttribute('description')))
    crossFileTest.appendChild(element)

    element = doc.createElement('Criticality')
    element.appendChild(doc.createTextNode(crossFileTestNode.getAttribute('criticality')))
    crossFileTest.appendChild(element)

    expressionNode = crossFileTestNode.getElementsByTagNameNS(NSS['ct'],'ValueExpressionDbl')[0]
    element = doc.createElement('Value_Expression_Dbl')
    element.appendChild(doc.createCDATASection(expressionNode.firstChild.nodeValue))
    crossFileTest.appendChild(element)

    expressionNode = crossFileTestNode.getElementsByTagNameNS(NSS['ct'],'ValueExpressionHdr')[0]
    element = doc.createElement('Value_Expression_Hdr')
    element.appendChild(doc.createCDATASection(expressionNode.firstChild.nodeValue))
    crossFileTest.appendChild(element)

    loCrossFileTests.appendChild(crossFileTest)


def setCount():
    """
       Get all the elements which hold a List_of. Count the
       ammount of childeren and set the count attribute.
    """

    global templateDoc

    list  = templateDoc.getElementsByTagName('List_of_Named_Tests')
    list += templateDoc.getElementsByTagName('List_of_Named_Cross_File_Tests')
    list += templateDoc.getElementsByTagName('List_of_Test_Definitions')
    list += templateDoc.getElementsByTagName('List_of_Tests')
    list += templateDoc.getElementsByTagName('List_of_Named_Test_References')
    list += templateDoc.getElementsByTagName('List_of_Cross_File_Tests')
    list += templateDoc.getElementsByTagName('List_of_Named_Cross_File_Test_References')

    for node in list:
        count = node.childNodes.length

        node.setAttribute('count', str(count))


def modifyHeader():
    """
       If the template tag File_Name is empty, insert the output file name.
    """
    global templateDoc

    doc = templateDoc.documentElement.ownerDocument
    fileNameE = templateDoc.getElementsByTagName('File_Name')
    if fileNameE[0].hasChildNodes()!= True:
        fileNameText = doc.createTextNode(File_Name)
        fileNameE[0].appendChild(fileNameText)


def main():
    """
       Function to handle all the actions taken by this script.
    """

    global tempdoc

    getArgs()
    setPathVars()
    testFiles()
    getAndSetupDocs()
    getProductList()

    for entry in productList:
        productType = entry[0]
        productId = entry[1]
        productVersion = entry[2]

        loTests, loNamedTRef, loCrossFileTests, loNamedCFTRef = setupTestDefinition(productType, productVersion)

        tempDoc= collectId(productId)
        doc = tempDoc.documentElement.ownerDocument

        list = doc.getElementsByTagNameNS(NSS['ct'], 'NamedTest')
        for entry in list:
            handleNamedTestReference(entry, loNamedTRef)

        list = doc.getElementsByTagNameNS(NSS['ct'], 'NamedCrossFileTest')
        for entry in list:
            handleNamedCrossFileTestReference(entry, loNamedCFTRef)

        list = doc.getElementsByTagNameNS(NSS['ct'], 'Test')
        for entry in list:
            handleTest(entry, loTests)

        list = doc.getElementsByTagNameNS(NSS['ct'], 'CrossFileTest')
        for entry in list:
            handleCrossFileTest(entry, loCrossFileTests)

    modifyHeader()
    setCount()
    print >>outputHandle, templateDoc.toxml()
    outputHandle.close()
    command = 'xmllint --format --output '+ outputPath + ' ' + outputPath
    os.system(command)



"""
   Run this script...
"""
main()

