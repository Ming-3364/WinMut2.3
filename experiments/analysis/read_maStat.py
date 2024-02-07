import sys
import traceback


try:
    filename = sys.argv[1]

    tot_num_OR_func = 0
    tot_num_OR_inst = 0
    tot_num_arich_32 = 0
    with open(filename) as f:
        for l in f:
            if l:
                l = l.strip()
                tot_num_OR_func += (int)(l.split(" ")[0])
                tot_num_OR_inst += (int)(l.split(" ")[1])
                tot_num_arich_32 += (int)(l.split(" ")[2])
    print(tot_num_OR_func, tot_num_OR_inst, tot_num_arich_32, f"{tot_num_arich_32/tot_num_OR_inst*100:>.2f%}")

except Exception as e:
    traceback.print_exc(file=sys.stdout)