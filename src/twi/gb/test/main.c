//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
// The process start point for testing twi-gb.
//-----------------------------------------------------------------------
//=======================================================================
#include <stdio.h>
#include <twi/gb/log.h>
#include <twi/gb/test.h>

int main() {
	twi_gb_log_create();
	size_t fail_count;
	if (fail_count = twi_gb_test_cpu_all())
		fprintf(stderr, "%zu CPU test%s failed.\n", fail_count, (fail_count != 1 ? "s" : ""));
	else
		fprintf(stderr, "All CPU tests successful.\n");
	twi_gb_log_delete();
} // end main()

