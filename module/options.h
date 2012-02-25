/*
 *  options.h - eee PC CPU speed control tool compilation options
 *
 *  Copyright (C) 2012 Olli Vanhoja
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Ths program is distributed in the hope that it will be useful,
 *  but WITOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTAILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Template Place, Suite 330, Boston, MA  02111-1307 USA
 *  
 *  ---------
 *
 *  This code comes WITHOUT ANY WARRANTY whatsoever.
 */

#define EEEFSB_MINFSB        150   // Minimum FSB allowed [MHz]
#define EEEFSB_MAXFSB        570   // Maximum FSB allowed [MHz]
#define EEEFSB_MULSTEP       1     // Maximum FSB multiplier step per jiffy
#define EEEFSB_HIVOLTFSB     85    // FSB value when high voltage is needed
#define EEEFSB_STEPDELAY     200   // Delay between steps [jiffy]
#define EEEFSB_CPU_M_SAFE    50
#define EEEFSB_CPU_N_SAFE    420
#define EEEFSB_PLL_CONST_MUL 16    // Should be 24 not 16??
#define EEEFSB_CPU_MUL       12    // From datasheet
#define EEEFSB_PCI_SAFE      15
