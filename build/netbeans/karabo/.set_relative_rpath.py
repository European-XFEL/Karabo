#! /usr/bin/env python
# -*- coding: iso-8859-1 -*-

#
#   set_relative_rpath: entries for a program and its private libraries
#   Copyright (C) 2012 science+computing ag
#   Author: Michael Bauer
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, see <http://www.gnu.org/licenses/>.
#

##################################################################################
## This version of the script is for treating already copied/relocated          ##
## installation trees obtained from RPMs installed inside the real system       ##
##################################################################################

import glob
import optparse
import os
import os.path
import stat
import re
import shutil
import string
import StringIO
import subprocess
import logging
import collections
import sys

_LOGGER = logging.getLogger("rpath")
#_LOGGER.setLevel("DEBUG")


##################################################################################
## Functions for sorting the files in the directory according to their filetype ##
##################################################################################

def getFileList(startpath):
    """Returns a recursive list of all files in a directory;
    if the given startpath corresponds to a single file, returns its path
    encapsulated as a list"""
    if os.path.isfile(startpath):
        return [os.path.normpath(startpath)]
    else:
        liste = []
        islink = os.path.islink
        join = os.path.join
        for root, dirs, files in os.walk(startpath):
            for name in files:
                file_ = join(root, name)
                if islink(file_):
                    continue
                liste.append(file_)
        return liste

def runFile(filename):
    """ run "file" on the specified file and return the output"""
    file = subprocess.Popen(["file", filename], \
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return file.communicate()[0]


def determineFileTypes(file_output_list):
    """  input: list of file-outputs as produced by e.g. map(runFile, getFileList(startpath)))
    sorts the files into different lists according to their file types
    return value: tuple of (elf-files, text-files, other-files)"""
    elflist = []
    textlist = []
    otherlist = []
    for file in file_output_list:
        if re.search("ELF .* (executable|shared object).*dynamically linked", file):
            elflist.append(re.match('(.*?):', file).group(1))
        elif re.search("[ \t\n\r\f\v]text[ \t\n\r\f\v\,]", file):
            # the comma is taken into account as a trailing separator so that also files
            # categorized as "ASCII text, with very long lines" (e.g., Qt prl files)
            # are recognized as text files
            textlist.append(re.match('(.*?):', file).group(1))
        else:
            otherlist.append(re.match('(.*?):', file).group(1))
    return (elflist, textlist, otherlist)

######################################
## Functions for treating ELF files ##
######################################

def extractLibNameLdd(s):
    """ Helper function for getLibNames: extracts the pathname of a library
    from the output of ldd"""
    found = re.search("=> (.*) \(", s)
    if found is not None:
        return found.group(1)
    else:
        return None

def extractLibNameLdconf(s):
    """ Helper function for getExclusionList: extracts the pathname of a library
    from the output of ldconf"""
    found = re.search("=> (.*)", s)
    if found is not None:
        return found.group(1)
    else:
        return None


def getLibNames(filename):
    """ Returns the list of libraries required by the given file"""
    # TODO: This way, only the rpaths of executables which actually use it
    #   will be changed. If a file contains an absolute rpath but does not depend
    #   on the libraries located there, its rpath will not be modified.
    #   Maybe this behaviour should be changed - just in case?
    ldd = subprocess.Popen(["ldd", filename], \
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    lddres = ldd.communicate() # return value: tuple (stdout, atderr)
    ldds = StringIO.StringIO(lddres[0]) # elegantere Methode?
    liblist = map(extractLibNameLdd, ldds)
    return [os.path.realpath(x) for x in liblist if x is not None and x is not '']
    #return [x for x in liblist if x is not None and x is not '']

def getExclusionList(get_it):
    """Reads and returns the exclusion list of system libraries from the ldconfig cache"""
    if get_it:
        ldconf = subprocess.Popen(["/sbin/ldconfig", "-p"], \
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ldconfres = ldconf.communicate()
        ldconfs = StringIO.StringIO(ldconfres[0])
        liblist = map(extractLibNameLdconf, ldconfs)
        return [os.path.realpath(x) for x in liblist if x is not None and x is not '']
    else:
        return []

def getExclusionPaths(filename):
    """ Reads and returns the exclusion list of paths to system libraries from the given file"""
    try:
        f = open(filename, 'r')
    except IOError, e:
        if filename == "":
            _LOGGER.info("INFO: No exclusion file specified. No pathnames excluded.")
        else:
            _LOGGER.warn("WARNING: Problem reading exclusion file %r (%s) ! No pathnames excluded.", filename, e)
        return []
    else:
        liste = map(string.strip, f.readlines())
        # the following is the handling of the "include" statement in /etc/ld.so.conf
        # without that, the following line would be sufficient:
        # return map(string.strip, f.readlines())
        for i in liste[:]:
            if re.match("include ", i):
                liste.remove(i)
                for j in glob.glob(os.path.join(os.path.dirname(filename), re.search("include (.*)", i).group(1))):
                    liste.extend(getExclusionPaths(j))
        f.close()
        return liste
    # NOTE: does not (yet?) parse the optional library type which can be
    #   specified in /etc/ld.so.conf according to http://www.supernature-forum.de/alles-rund-um-linux/19073-ldconfig.html
    # but this seems to be relevant mainly for libc4, libc5 or libc6
    #   of which the former two are severely out of date
    #   -> to be implemented only if necessary

def classifyLibraries(liblist, basepath, filelist, exliblist, expathlist):
    """ Classifies the libraries in liblist into the following three categories:
    1. Libraries for which the rpath should be changed because they are inside the basepath
    2. Libraries for which the rpath should not be changed because their path
       is inside the exclusion list (exlist), e.g., system libraries
    3. Libraries for which a warning should be generated because they are neither
       inside the base path nor excluded by the exlist
    Returns a tuple of the libraries in liblist sorted into these three categories"""
    change = []
    good = []
    bad = []
    for lib in liblist:
        if os.path.join(basepath, lib) in filelist: # library included in the available fileset
            change.append(os.path.abspath(lib))
        elif lib in exliblist or os.path.dirname(lib) in expathlist: # system library
            good.append(os.path.abspath(lib))
        else:
            bad.append(os.path.abspath(lib))
        # the original code no longer applies when treating already relocated RPM contents
        # if os.path.dirname(lib) in exlist:  # system library
        #     good.append(os.path.abspath(lib))
        # elif os.path.commonprefix([os.path.abspath(basepath), os.path.abspath(lib)]) == os.path.abspath(basepath):
        #     change.append(os.path.abspath(lib))
        # else:                               # non-system library outside basepath
        #     bad.append(os.path.abspath(lib))
    return (change, good, bad)                  

def generatePathset(relevantlist, filename, basepath):
    """Condenses the pathnames of the files in the relevantlist into a set of pathnames
    relative to the given filename and returns this set"""
    # NOTE: relpath is available only from Python 2.6 on
    # return set([os.path.relpath(os.path.join(basepath,i), os.path.dirname(filename)) for i in [os.path.dirname(j) for j in relevantlist]])
    assert os.path.isabs(filename)
    assert os.path.isabs(basepath)

    pathSet = set()
    filedir = os.path.dirname(filename)
    for lib in relevantlist:
        absolutelib = os.path.join(basepath, lib)
        libdir = os.path.dirname(absolutelib)
        relativelibdir = os.path.relpath(libdir, filedir)
        pathSet.add(relativelibdir)
    _LOGGER.debug("rpath for %r: %r", filename, pathSet)
    return pathSet
        
def normRpathDir(rdir):
    """return a normalized form of an rpath directory."""
    if "" == rdir or "$ORIGIN" == rdir:
        # already normal
        return rdir
    if rdir.startswith("$ORIGIN/"):
        p = os.path.normpath("." + rdir[7:])
        if "." == p:
            return "$ORIGIN"
        return "$ORIGIN/" + p
    if rdir.startswith("$ORIGIN"):
        _LOGGER.warn("Strange rpath entry: %r", rdir)
        return rdir    
    return os.path.normpath(rdir)

def getPreviousRpath(filename):
    """Returns an existing rpath entry"""
    per = subprocess.Popen(["patchelf", "--print-rpath", filename], \
            stdout=subprocess.PIPE)
    perres = per.communicate()
    perstring = perres[0].strip()
    rpath = [normRpathDir(d) for d in perstring.split(os.pathsep) if d]
    _LOGGER.debug("Previous rpath for %r: %r", filename, rpath)
    return rpath

def getPreviousRelativeRpath(filename):
    """Returns those parts of an existing rpath entry of file which are relative paths"""
    perpaths = getPreviousRpath(filename)
    return [x for x in perpaths if x.startswith('$ORIGIN')]

def constructRpath(file, pathset):
    """constructs an rpath-string from a set of pathnames"""

    if not isinstance(pathset, collections.Sequence):
        # pathset has no order. In order to get deterministic
        # results, we sort pathset
        pathset = list(pathset)
        pathset.sort()

    # keep old relative rpath entries
    rpath = getPreviousRelativeRpath(file)
    for d in pathset:
        d = normRpathDir(os.path.join("$ORIGIN", d))
        if d not in rpath:
            rpath.append(d)
    return rpath

def cleanupRpath(filename, filelist, exliblist, expathlist, basepath, warn):
    """Returns an rpath for a given file, that does not contain directories from the exclusion list."""
    rpath = getPreviousRpath(filename)
    return [d for d in rpath if d not in expathlist]

def generateRpath(filename, filelist, exliblist, expathlist, basepath, warn):
    """Returns the rpath for a given file (taking into account the
    exclusion list and the basepath)
    If warn is True, warnings are printed if the file links to libraries
    which are neither excluded by the exlist nor located inside the basepath"""
    cgb = classifyLibraries(getLibNames(filename), basepath, filelist, exliblist, expathlist)
    if (warn == True and len(cgb[2])):
        for lib in cgb[2]:
            _LOGGER.warn("File %r links with non-standard library %r outside the base path %r. Please fix rpath manually.",
                         os.path.abspath(filename),
                         os.path.abspath(lib),
                         os.path.abspath(basepath))
    return constructRpath(filename, generatePathset(cgb[0], filename, basepath))

def runPatchelf(filename, rpath):
    """Sets the rpath of the specified filename using the patchelf command
    leaving the inode number intact."""
    if not isinstance(rpath, basestring):
        rpath = os.pathsep.join(rpath)
    tmpfile = filename + ".to_be_patched"
    try:
        os.chmod(filename, os.stat(filename).st_mode | stat.S_IWUSR | stat.S_IWRITE)
        shutil.copy2(filename, tmpfile)
        subprocess.check_call(["patchelf", "--force-rpath", "--set-rpath", rpath, tmpfile])
        if os.path.basename(filename).startswith("libpython%s.%s.so.1" % sys.version_info[:2]):
            # we can't modifiy the shared lib, that we are currently using
            # therefore we must change the Inode
            os.rename(tmpfile, filename)
        else:
            shutil.copyfile(tmpfile, filename)
            os.remove(tmpfile)
    except Exception:
        _LOGGER.exception("WARNING: cannot modify file %r", filename)
        raise
    else:
        _LOGGER.debug("Set rpath for %r to %r", filename, rpath)

#######################################
## Functions for treating test files ##
#######################################

def replaceBasePath(line, basepath):
    """ Replaces all occurrences of the absolute path of basepath in line by a '.' """
    ## this has to be modified for the correct treatment of copied RPM trees
    return string.replace(line, os.path.abspath(basepath), '.')

######################################################################
## Functions for treatment of the files according to their filetype ##
######################################################################

def runElfTreatment(elflist, filelist, exliblist, expathlist, basepath, warn):
    """ Treatment of the ELF files listed in elflist by runPatchelf
    Note: all files in elflist are assumed to be ELF files!"""
    _LOGGER.info("elf treatment: cleaning up RPATH entries ...")
    for file in elflist:
        runPatchelf(file, cleanupRpath(file, filelist, exliblist, expathlist, basepath, warn))
    _LOGGER.info("elf treatment: cleanup is done. Creating relative RPATH entries ...")
    for file in elflist:
        runPatchelf(file, generateRpath(file, filelist, exliblist, expathlist, basepath, warn))
    _LOGGER.info("elf treatment: done.")

def runTextTreatment(filelist, basepath):
    """ Treatment of the text files listed in filelist by replaceBasePath
    Note: all files in filelist are assumed to be text files!"""
    for file in filelist:
        input = open(file, "r")
        input_lines = input.readlines()
        input.close()
        output_lines = []
        for line in input_lines:
            output_lines.append(replaceBasePath(line, basepath))
        if (output_lines != input_lines):
            output = open(file, "w")
            output.writelines(output_lines)
            output.close()


def runOtherTreatment(filelist, basepath):
    """ Treatment of the unspecified files listed in filelist
    by printing warning messages if they contain the absolute pathname of basepath"""
    for file_ in filelist:
        # ignore certain files, that always contain the path
        if os.path.splitext(file_)[1] in ('.pyc', '.pyo'):
            continue
        input = open(file_, "rb")
        input_contents = input.read()
        if re.search(os.path.abspath(basepath), input_contents):
            _LOGGER.warn("Absolute path %r in %r", os.path.abspath(basepath), file_)
    pass

####################
## main functions ##
####################

def main():
    """Main function for non-interactive use"""
    usage = "usage: %prog [options] startpath\n\n  Set the rpath of all ELF executables and shared objects found below\n  the startpath to relative pathnames"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("-c", "--configfile", dest="exfilename", default="/etc/ld.so.conf", help="List of library search paths not to be included in the rpath (default: %default)")
    parser.add_option("-l", "--ldconfig-cache", dest="use_ldconfcache", default=True, action="store_true", help="Use the ldconfig cache to determine which libraries are to be treated as system libraries")
    parser.add_option("-n", "--no-ldconfig-cache", dest="use_ldconfcache", action="store_false", help="Do not use the ldconfig cache to determine which libraries are to be treated as system libraries")
    parser.add_option("-q", "--no-warnings", dest="warn", action="store_false", help="Suppress warnings for linking with non-system libraries outside the startpath")
    parser.add_option("-w", "--show-warnings", dest="warn", action="store_true", default=True, help="Show warnings for linking with non-system libraries outside the startpath (default)")
    parser.add_option("-t", "--replace-text", dest="text", action="store_true", default=False, help="Replace absolute basepath strings in textfiles by dots")
    parser.add_option("-o", "--show-other-files", dest="other", action="store_true", default=False, help="Show warnings if the absolute basepath is contained in files of other types which cannot be treated automatically by this program")
    (options, args) = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    if len(args) != 1 or not os.path.exists(os.path.realpath(args[0])):
        parser.error("no valid pathname specified")
    basepath = os.path.abspath(args[0])
    filelist = getFileList(basepath)
    filetypes = determineFileTypes(map(runFile, filelist))
    runElfTreatment(filetypes[0], filelist, getExclusionList(options.use_ldconfcache), getExclusionPaths(options.exfilename), basepath, options.warn)
    if (options.text):
        runTextTreatment(filetypes[1], basepath)
    if (options.other):
        runOtherTreatment(filetypes[2], basepath)
    
if __name__ == "__main__":
    main()
