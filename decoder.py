# python implementation of the HID Corporate 1000 RFID format
# Copyright (C) 2013 Peter Chinetti
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
import argparse

def decode(code):
    if check_parity(code):
        print("\tParity Check Successful!")
        print("\t\tFacility Code:\t",int(facility_code(code),2))
        print("\t\tCard Code:\t",int(card_code(code),2))
    else:
        print("\tParity Check Failed.")
def check_parity(code):
    if len(code) is not 35:
        return False
    bool_code = [int(x) for x in list(code)]
    return full_parity(bool_code) and odd_parity(bool_code) and even_parity(bool_code)

def facility_code(code):
    return code[2:14]

def card_code(code):
    return code[14:34]

def full_parity(code):
    a = 0
    for b in code:
        a += b
    return bool(a % 2)

def even_parity(code):
    a = 0
    for bit in [x-1 for x in [2,3,4,6,7,9,10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,34]]:
        a += code[bit]
    return not bool(a % 2)

def odd_parity(code):
    a = 0
    for bit in [x-1 for x in [2,3,5,6,8,9,11,12,14,15,17,18,20,21,23,24,26,27,29,30,32,33,35]]:
        a += code[bit]
    return bool(a % 2)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--file",type=str,required=True,help="Filename of csv file w/ rows of: \n<card# (binary)>,<card holder>")
    args = parser.parse_args()
    for line in open(args.file):
        (code,name) = [x.strip() for x in line.split(",")]
        print(name+":")
        decode(code)
