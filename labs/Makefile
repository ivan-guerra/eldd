SUBDIRS = lab_3_1 lab_3_2 lab_3_3 \
					lab_4_1 lab_4_2 lab_4_3 \
					lab_5_1 lab_5_2 lab_5_3 lab_5_4 \
					lab_6_1 \
					lab_7_1 lab_7_2 lab_7_3 \
					lab_8_1 \
					lab_9_1

all: subdirs

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
