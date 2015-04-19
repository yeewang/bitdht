/*
 * bitdht/bdmetric_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bdpeer.h"
#include "bitdht/bdstddht.h"
#include <iostream>
#include <stdio.h>


#include "utest.h"

bool test_metric_explicit();
bool test_metric_random();

INITTEST();

int main(int argc, char **argv)
{
        LOG << log4cpp::Priority::INFO << "libbitdht: " << argv[0] << std::endl;

	test_metric_explicit();

        FINALREPORT("libbitdht: Metric Tests");
        return TESTRESULT();
}

bool test_metric_explicit()
{
        LOG << log4cpp::Priority::INFO << "test_metric_explicit:" << std::endl;

#define NUM_IDS 6

	/* create some ids */
	bdId id[NUM_IDS];
	int i, j;

	/* create a set of known ids ... and 
	 * check the metrics are what we expect.
	 */

	for(i = 0; i < NUM_IDS; i++)
	{
		bdZeroNodeId(&(id[i].id));
	}

	/* test the zero node works */
	for(i = 0; i < NUM_IDS; i++)
	{
		for(j = 0; j < BITDHT_KEY_LEN; j++)
		{
        		CHECK(id[i].id.data[j] == 0);
		}
	}

	for(i = 0; i < NUM_IDS; i++)
	{
		for(j = i; j < NUM_IDS; j++)
		{
			id[j].id.data[BITDHT_KEY_LEN - i - 1] = 1;
		}
	}
		
	for(i = 0; i < NUM_IDS; i++)
	{
		LOG.info("id[%d]:", i+1);
		bdStdPrintId(LOG << log4cpp::Priority::INFO,&(id[i]));
		LOG.info("\n");
	}

	/* now do the sums */
	bdMetric met;
	bdMetric met2;
	int bdist = 0;

	for(i = 0; i < 6; i++)
	{
		for(j = i+1; j < 6; j++)
		{
			bdStdDistance(&(id[i].id), &(id[j].id), &met);

			LOG.info("%d^%d:", i, j);
			bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met);
			LOG.info("\n");

			bdist = bdStdBucketDistance(&met);
			LOG.info(" bucket: %d\n", bdist);
		}
	}

#if 0
	int c1 = met < met2;
	int c2 = met2 < met;

	LOG.info("1^2<1^3? : %d  1^3<1^2?: %d\n", c1, c2);
#endif


        REPORT("Test Byte Manipulation");
        //FAILED("Couldn't Bind to socket");

	return 1;
}



bool test_metric_random()
{
        LOG << log4cpp::Priority::INFO << "test_metric_random:" << std::endl;

	/* create some ids */
	bdId id1;
	bdId id2;
	bdId id3;
	bdId id4;
	bdId id5;
	bdId id6;

	bdStdRandomId(&id1);
	bdStdRandomId(&id2);
	bdStdRandomId(&id3);
	bdStdRandomId(&id4);
	bdStdRandomId(&id5);
	bdStdRandomId(&id6);

	LOG.info("id1:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id1);
	LOG.info("\n");

	LOG.info("id2:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id2);
	LOG.info("\n");

	LOG.info("id3:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id3);
	LOG.info("\n");

	LOG.info("id4:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id4);
	LOG.info("\n");

	LOG.info("id5:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id5);
	LOG.info("\n");

	LOG.info("id6:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id6);
	LOG.info("\n");

	/* now do the sums */
	bdMetric met;
	bdMetric met2;
	int bdist = 0;
	bdStdDistance(&(id1.id), &(id2.id), &met);

	LOG.info("1^2:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met);
	LOG.info("\n");
	bdist = bdStdBucketDistance(&met);
	LOG.info(" bucket: %d\n", bdist);

	bdStdDistance(&(id1.id), &(id3.id), &met2);
	bdist = bdStdBucketDistance(&met2);

	LOG.info("1^3:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met2);
	LOG.info("\n");
	LOG.info(" bucket: %d\n", bdist);

	int c1 = met < met2;
	int c2 = met2 < met;

	LOG.info("1^2<1^3? : %d  1^3<1^2?: %d\n", c1, c2);


	bdStdDistance(&(id1.id), &(id4.id), &met2);
	bdist = bdStdBucketDistance(&met2);

	LOG.info("1^4:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met2);
	LOG.info("\n");
	LOG.info(" bucket: %d\n", bdist);

	c1 = met < met2;
	c2 = met2 < met;

	LOG.info("1^2<1^4? : %d  1^4<1^2?: %d\n", c1, c2);

	bdStdDistance(&(id1.id), &(id5.id), &met);
	bdist = bdStdBucketDistance(&met);

	LOG.info("1^5:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met);
	LOG.info("\n");
	LOG.info(" bucket: %d\n", bdist);

	bdStdDistance(&(id1.id), &(id6.id), &met);
	bdist = bdStdBucketDistance(&met);

	LOG.info("1^6:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met);
	LOG.info("\n");
	LOG.info(" bucket: %d\n", bdist);

	bdStdDistance(&(id2.id), &(id3.id), &met);
	bdist = bdStdBucketDistance(&met);

	LOG.info("2^3:");
	bdStdPrintNodeId(LOG << log4cpp::Priority::INFO,&met);
	LOG.info("\n");
	LOG.info(" bucket: %d\n", bdist);


	LOG.info("id1:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id1);
	LOG.info("\n");

	LOG.info("id2:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id2);
	LOG.info("\n");

	LOG.info("id3:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id3);
	LOG.info("\n");

	LOG.info("id4:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id4);
	LOG.info("\n");

	LOG.info("id5:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id5);
	LOG.info("\n");

	LOG.info("id6:");
	bdStdPrintId(LOG << log4cpp::Priority::INFO,&id6);
	LOG.info("\n");


	return 1;
}


