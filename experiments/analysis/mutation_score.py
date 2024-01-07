import sys
import traceback
import os
from enum import Enum, auto
import re

class MutationStatus(Enum):
    KILLED_BY_PROC_OUTPUT = auto()          # 程序输出
    KILLED_BY_PROC_END_STATUS = auto()      # 程序结束状态

    SURVIVED_NOT_AFFECT_STATUS = auto()     # 未影响状态
    SURVIVED_NOT_AFFECT_OUTPUT = auto()     # 未影响输出
    SURVIVED_NOT_COVERED = auto()           # 未参与运行

KILLED = [
            MutationStatus.KILLED_BY_PROC_END_STATUS,
            MutationStatus.KILLED_BY_PROC_OUTPUT
         ]

SURVIVED = [
            MutationStatus.SURVIVED_NOT_AFFECT_STATUS,
            MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT,
            MutationStatus.SURVIVED_NOT_COVERED
           ]



class Mutation:
    def __init__(self, m_type, sop, op_0, op_1, op_2):
        self.m_type = m_type
        self.sop = sop
        self.op_0 = op_0
        self.op_1 = op_1
        self.op_2 = op_2

        self.not_covered = True
        self.mutation_status = []

class ProcEndStatus(Enum):
    SAME_WITH_ORI = auto()
    EXITED = auto()
    SIGNALED = auto()

class ProcTreeNode:
    # TODO: mut_id_list 使用数组，或许可以换种高级方式
    def __init__(self, eq_class_tuple_list, proc_end_status, proc_exit_or_signal_val=0):
        self.eq_class_tuple_list = eq_class_tuple_list
        self.proc_end_status = proc_end_status
        if proc_end_status != ProcEndStatus.SAME_WITH_ORI:
            self.proc_exit_or_signal_val = proc_exit_or_signal_val

# MA : mutation analysis
class MACase:
    def __init__(self, case_dir) -> None:
        self.case_dir          = case_dir
        self.all_mutation_path = os.path.join(case_dir, "all_mutation")
        self.proc_tree_path    = os.path.join(case_dir, "proc_tree")

        self.all_mutation = self.getAllMutation(self.all_mutation_path)

        self.proc_tree = self.readProcTree(self.proc_tree_path)

        
        self.cal_mutation_score()

    def cal_mutation_score(self):
        self.checkProcEndStatus()
        self.checkProcOutput()

        self.all_mutation_num = len(self.all_mutation) - 1
        self.killed_mutation_num = 0

        # 基于分流执行的方法，对一个 mut_id，可能会出现多种状态，以 killed 为准，
        # 多个 killed 原因可以共存，但记录在 survived 中的 mut_id 不能有 killed
        self.mutation_status_statistics = {
            MutationStatus.KILLED_BY_PROC_OUTPUT        : set(),
            MutationStatus.KILLED_BY_PROC_END_STATUS    : set(),
            MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT   : set(),
            MutationStatus.SURVIVED_NOT_AFFECT_STATUS   : set(),
            MutationStatus.SURVIVED_NOT_COVERED         : set(),

        }
        self.mutation_status_statistics_killed_set = set()
        self.mutation_status_statistics_survived_set = set()
        # self.killed_by_proc_output_set = set()

        # self.killed_by_proc_end_status_set = set()
        # self.survivied_not_affect_status_set = set()
        # self.survivied_not_affect_output_set = set()
        # self.survivied_not_covered = set()
        for mut_id, mutation in enumerate(self.all_mutation):
            if mut_id == 0:
                continue
            else:
                assert isinstance(self.all_mutation[mut_id], Mutation) and "cal_mutation_score: not a Mutation class\n"
                # 前置，添加 MutationStatus.SURVIVED_NOT_COVERED 
                if self.all_mutation[mut_id].not_covered:
                    self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_COVERED)
                
                for mutation_status in self.all_mutation[mut_id].mutation_status:
                    if mutation_status in KILLED:
                        self.killed_mutation_num += 1
                        break
                self.classifyMutationStatus(mut_id, mutation)
        self.checkClassifyConsistency()
    
    def classifyMutationStatus(self,mut_id,  mutation):
        # assert isinstance(mutation, Mutation) and "classifyMutationStatus: not a Mutation class\n"
        killed_flag = False
        for mutation_status in self.all_mutation[mut_id].mutation_status:
            if mutation_status in KILLED:
                self.mutation_status_statistics[mutation_status].add(mut_id)
                self.mutation_status_statistics_killed_set.add(mut_id)
                killed_flag = True
        if not killed_flag:
            for mutation_status in self.all_mutation[mut_id].mutation_status:
                self.mutation_status_statistics[mutation_status].add(mut_id)
                self.mutation_status_statistics_survived_set.add(mut_id)
    
    def checkClassifyConsistency(self):
        # 1. 出现在 killed中的 mut_id 必定不能出现在 SURVIVED_NOT_COVERED 中
        for mut_id in self.mutation_status_statistics_killed_set:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        # 2. 出现在 SURVIVED_NOT_COVERED 中的 mut_id，其对应的 mutation 应当只有这一条
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]:
            assert len(self.all_mutation[mut_id].mutation_status) == 1
        # 3. KILLED 的所有 flag 可以和除了 SURVIVED_NOT_COVERED 共存    （已被 1 检查）
        # 4. SURVIVED 中，NOT_AFFECT_STATUS 可以和 NOT_AFFECT_OUTPUT 共存，但不能和其他共存
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT]:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        # 5. killed + survived = all_mutation
        assert len(self.mutation_status_statistics_killed_set) + len(self.mutation_status_statistics_survived_set) \
                == len(self.all_mutation) - 1
        
    def __str__(self) -> str:
        ret = self.case_name
        ret += "\t" + f"mutation score: {self.killed_mutation_num}/{self.all_mutation_num}({self.killed_mutation_num / self.all_mutation_num *100:>.2f}%)"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"KILLED_BY_PROC_OUTPUT: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])}"
        # ret += f" : {self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT]}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"KILLED_BY_PROC_END_STATUS: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS])}"
        # ret += "\n"

        ret += "\t\t"
        ret += f"KILLED_BY_BOTH: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] & self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_AFFECT_STATUS: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_AFFECT_OUTPUT: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT])}"
        # ret += "\n"

        ret += "\t\t"
        ret += f"SURVIVED_BY_BOTH: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT] & self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_COVERED: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED])}"
        ret += f" : {self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"total: {len(self.all_mutation) - 1} \
                 killed: {len(self.mutation_status_statistics_killed_set)} \
                 survivied: {len(self.mutation_status_statistics_survived_set)}"
        ret += "\n"
        return ret


    def readProcTree(self, proc_tree_path):
        proc_tree = []
        with open(proc_tree_path, encoding='utf-8', errors='replace') as f:
            for l in f:
                l = l.strip()
                if l.startswith('--'):
                    self.case_name = '--'.join(l.split('--')[2:]).strip()
                    continue
                elif l.startswith('++'):
                    continue
                elif l.startswith("accmut::ori_exit_val:"):
                    self.ori_end_status = ProcEndStatus.EXITED
                    self.ori_exit_or_signal_val = int(l.split("accmut::ori_exit_val:")[1].strip())
                elif l.startswith("reduced:"):          # reduced with ori (0)
                    eq_class_reduced_str = l.split("reduced:")[1].strip()
                    eq_class_reduced_tuple_list = self.getEqClass(eq_class_reduced_str)

                    # self.getProcFromEqClass(eq_class_reduced_tuple_list, ProcEndStatus.SAME_WITH_ORI)
                    proc_tree.append(ProcTreeNode(eq_class_reduced_tuple_list, ProcEndStatus.SAME_WITH_ORI))
                else:
                    assert l[0] in '0123456789' and f"{l} not a proc\n"
                    fork_from = int(l.split("=>")[0].strip())
                    eq_from   = int(l.split("=>")[1].split(":")[0].strip())
                    eq_to	  = int(l.split("=>")[1].split(":")[1].strip())
                    mut_id_in_all_mutation    = int(l.split(":")[2].split("(")[0].strip())
                    mut_str_in_module = l.split("(")[1].split(")")[0].strip()
                    proc_end_status_str = l.split("):")[1].split("accmut::eq_class:")[0].split("/")[1].strip()
                    eq_class_str = l.split("accmut::eq_class:")[1].strip()

                    eq_class_tuple_list = self.getEqClass(eq_class_str)
                    proc_end_status, proc_exit_or_signal_val = self.getProcEndStatus(proc_end_status_str)

                    proc_tree.append(ProcTreeNode(eq_class_tuple_list, proc_end_status, proc_exit_or_signal_val))
        return proc_tree
    
    def getOutputNeedCheck(self):
        contents = os.listdir(self.case_dir)
        file_need_check   = []
        mut_id_need_check = []
        for content in contents:
            if content.startswith("mut_output-"):
                if content.startswith("mut_output-0-"):
                    assert os.path.isfile(os.path.join(self.case_dir, content))             \
                            and f"getOutputNeedCheck: {self.case_dir}/{content}not a file\n"
                    file_need_check.append(content.split("mut_output-0-")[1])
                else:
                    pattern = re.compile(r'^mut_output-(\d+)-(.*)$')
                    match_result = pattern.match(content)
                    assert match_result and f"{content} : not a mut output\n"
                    mut_id = int(match_result.group(1))
                    mut_id_need_check.append(mut_id)

        return file_need_check, mut_id_need_check


    def isSameContent(self, file1_path, file2_path):
        with open(file1_path, 'r') as file1, open(file2_path, 'r') as file2:
            content1 = file1.read()
            content2 = file2.read()
            return content1 == content2

    def checkProcOutput(self):
        file_need_check, mut_id_need_check = self.getOutputNeedCheck()

        for mut_id in mut_id_need_check:
            for output_file_name in file_need_check:
                ori_file = "mut_output-" + str(0) + "-" + output_file_name
                ori_file_path = os.path.join(self.case_dir, ori_file)
                cur_check_file = "mut_output-" + str(mut_id) + "-" + output_file_name
                cur_check_file_path = os.path.join(self.case_dir, cur_check_file)

                assert os.path.isfile(ori_file_path) and f"{ori_file_path}: should exist\n"

                assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcOutput: not a Mutation class\n"
                assert self.all_mutation[mut_id].not_covered == False and "if have output, it must be covered\n"

                if os.path.isfile(cur_check_file_path):
                    if self.isSameContent(ori_file_path, cur_check_file_path):
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT)
                    else:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_OUTPUT)
                else: 
                    self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_OUTPUT)

    def isSameProcEndStatusWithOri(self, proc):
        assert isinstance(proc, ProcTreeNode) and "isSameProcEndStatusWithOri: not a ProcTreeNode class\n"
        return (proc.proc_end_status == self.ori_end_status) and (proc.proc_exit_or_signal_val == self.ori_exit_or_signal_val)

    def checkProcEndStatus(self):
        for proc in self.proc_tree:
            assert isinstance(proc, ProcTreeNode) and "checkProcEndStatus: not a ProcTreeNode class\n"

            eq_class_mut_id = self.getIdListFromTupleList(proc.eq_class_tuple_list)
            proc_end_status = proc.proc_end_status
            
            for mut_id in eq_class_mut_id:
                if mut_id != 0:
                    assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcEndStatus: not a Mutation class\n"
                    self.all_mutation[mut_id].not_covered = False

                    if self.isSameProcEndStatusWithOri(proc) or proc_end_status == ProcEndStatus.SAME_WITH_ORI:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_AFFECT_STATUS)
                    else:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_END_STATUS)

            # if proc_end_status == ProcEndStatus.SAME_WITH_ORI:
            #     for mut_id in eq_class_mut_id:
            #         if mut_id != 0:
            #             assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcEndStatus: not a Mutation class\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]
            # else:
            #     if self.isSameProcEndStatus(proc, self.ori_end_status, self.ori_exit_or_signal_val):
            #         for mut_id in eq_class_mut_id:
            #             assert mut_id != 0 and "checkProcEndStatus: if need compare end status, it must not ori\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]
            #     else:
            #         for mut_id in eq_class_mut_id:
            #             assert mut_id != 0 and "checkProcEndStatus: if need compare end status, it must not ori\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.KILLED_BY_PROC_END_STATUS]

            

            
    
    def getIdListFromTupleList(self, eq_class_tuple_list):
        eq_class_mut_id = []
        for eq_class_tuple in eq_class_tuple_list:
            eq_class_mut_id.append(eq_class_tuple[0])
        return eq_class_mut_id

    def getProcEndStatus(self, proc_end_status_str):
        proc_end_status = None
        proc_exit_or_signal_val = None

        if proc_end_status_str.startswith("r"):
            proc_end_status = ProcEndStatus.EXITED
            proc_exit_or_signal_val = int(proc_end_status_str.split("r")[1].strip())
        elif proc_end_status_str.startswith("s"):
            proc_end_status = ProcEndStatus.SIGNALED
            proc_exit_or_signal_val = int(proc_end_status_str.split("s")[1].strip())
        else:
            assert False and f"{proc_end_status_str}: error process end status\n"

        return proc_end_status, proc_exit_or_signal_val

    def getProcFromEqClass(self, eq_class_tuple_list, proc_end_status, proc_end_status_str=""):
        pass

    def getEqClass(self, eq_class_str):             # [id : val] [id : val] [id : val] ...
        eq_class = []
        for raw_str in eq_class_str.split("]"):
            if raw_str.strip():
                eq_class_mut_id = int(raw_str.split(":")[0].split("[")[1].strip())
                eq_class_value = int(raw_str.split(":")[1].strip())
                eq_class_tuple = (eq_class_mut_id, eq_class_value)
                eq_class.append(eq_class_tuple)
        return eq_class

    def getAllMutation(self, all_mutation_path):
        all_mutation = []
        with open(all_mutation_path, encoding='utf-8', errors='replace') as f:
            for l in f:
                if l.startswith('--'):
                    continue
                elif l.startswith('++'):
                    continue
                else:
                    m_str = l.split(":")
                    assert len(m_str) == 5 and f"mutation error: {l}"
                    m_type = int(m_str[0].strip())
                    sop = int(m_str[1].strip())
                    op_0 = int(m_str[2].strip())
                    op_1 = int(m_str[3].strip())
                    op_2 = int(m_str[4].strip())
                    all_mutation.append(Mutation(m_type, sop, op_0, op_1, op_2))
        assert len(all_mutation) >= 1 and f"empty file: {all_mutation_path}"
        return all_mutation
        

# case_in_run = {
# 	case_0 : case_0_path,
# 	case_1 : case_1_path,
# 	...
# }
def readCaseInRun(runlog_dir):
    case_in_run = {}
    items = os.listdir(runlog_dir)
    items.sort()
    for item in items:
        item_path = os.path.join(runlog_dir, item)
        assert os.path.isdir(item_path) and f"{item_path} not a case dir!\n"
        case_in_run[item] = item_path
    return case_in_run
        

def cal_mutation_score(runlog_dir):
    case_in_run = readCaseInRun(runlog_dir)
    for case_dir in case_in_run.values():
        print(MACase(case_dir))
    
    
    

try:
    runlog_dir = sys.argv[1]
    cal_mutation_score(runlog_dir)
    
except Exception as e:
    traceback.print_exc(file=sys.stdout)