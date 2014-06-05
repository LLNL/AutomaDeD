##############################################################################
# Copyright (c) 2014, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
#
# This file is part of AutomaDeD.
# Written by:
# Ignacio Laguna:lagunaperalt1@llnl.gov, Subrata Mitra: mitra4@purdue.edu.
# All rights reserved.
# LLNL-CODE-647182
#
# For details, see https://github.com/scalability-llnl/AutomaDeD
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License (as published by
# the Free Software Foundation) version 2.1 dated February 1999.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
# conditions of the GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##############################################################################
import sys,codeEditor
from PyQt4.QtCore import *
from PyQt4.QtGui import *



def main():
	
	app = QApplication(sys.argv)
	window = QMainWindow()
	main_frame = QWidget()
	edit = codeEditor.PlainTextEdit()
        number_bar = codeEditor.NumberBar(edit)
	fileName = "/Users/mitra3/work/MPI_benchmarks/NPB3.3/NPB3.3-MPI/IS/is.c"
	cfile = QFile(fileName);
 	lines = QString()
	num_lines = 0
	if (cfile.open(QFile.ReadOnly | QFile.Text)):
		while(cfile.atEnd() == False):
			lines = lines + cfile.readLine()
			num_lines = num_lines + 1
			#print lines
	edit.setPlainText(lines);
	edit.setReadOnly(True);
	number_bar.adjustWidth(num_lines)
	edit.blockCountChanged.connect(number_bar.adjustWidth)
        edit.updateRequest.connect(number_bar.updateContents)

#following codes do not work... I was trying to scroll the cursor.. 
	#edit.moveCursor(QTextCursor.Start)
 	edit.highlight_line(111)
	#bottomRight = edit.viewport().rect().bottomRight();
	#edit.move(bottomRight.x(), bottomRight.y());
	edit.repaint()
	#edit.verticalScrollBar().setSliderPosition(100)
	edit.show()

	hbox = QHBoxLayout()
	hbox.setSpacing(10)
        hbox.setMargin(10)


	hbox.addWidget(number_bar)
	hbox.addWidget(edit)
	

	main_frame.setLayout(hbox)
	window.setCentralWidget(main_frame)

    	window.show()
    	app.exec_()
if __name__ == "__main__":
	main()
