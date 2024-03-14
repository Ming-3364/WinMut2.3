import sys
import traceback
import os
import re
from tqdm import tqdm 

class ChildProcOutput:
    def __init__(self, output_dict):
        self.output_dict = output_dict
        self.ordered_keys = sorted(self.output_dict.keys())

        # 遍历有序键列表，将值连接成一个bytes对象
        self.result_bytes = b''.join([self.output_dict[key] for key in self.ordered_keys])

    def __str__(self) -> str:
        ret = "{\n"
        for file_name, file_content in self.output_dict.items():
            ret += "\t{\n"
            ret += f"\t\tfile_name: {file_name}\n"
            ret += f"\t\tfile_content: {file_content}\n"
            ret += "\t}\n"
        ret += "}"
        return ret
    
    def __eq__(self, __value: object) -> bool:
        if isinstance(__value, ChildProcOutput):
            if self.result_bytes == __value.result_bytes:
                return True
        return False


class WriteCall:
    def __init__(self, target_file, content):
        self.target_file = target_file
        self.content = content

class Run:
    def __init__(self, run_log_dir, specified_case=None, specified_proc=[]):
        self.run_log_dir = run_log_dir
        self.case_in_run = self.readCaseInRun(run_log_dir)

        self.compare_output(self.case_in_run, specified_case, specified_proc)

    def compare_output(self, case_in_run, specified_case=None, specified_proc=[]):
        proc_forked = []
        proc_killed = []
        proc_error = []

        if specified_case:
            case_in_run = {specified_case: os.path.join(self.run_log_dir, specified_case)}
            print(case_in_run)
        for case_dir in tqdm(case_in_run.values(), desc="Cur Case: "):
            forked_path = os.path.join(case_dir, "forked")
            proc_forked, proc_killed, proc_error = self.readProcNeedCheck(forked_path)
            # assert len(proc_error) == 0
            proc_killed.append(0)

            if specified_case and len(specified_proc) != 0:
                proc_killed = specified_proc
            for proc in tqdm(proc_killed, desc="Cur Proc: ", leave=False, mininterval=10):

                output_from_file     = self.buildChildProcOutputFromFile(case_dir, proc)
                output_from_sysiolog = self.buildChildProcOutputFromSYSIOLog(case_dir, proc)
                
                if output_from_file != output_from_sysiolog:
                    print("====", os.path.basename(case_dir), proc, "====")
                    print(output_from_file)
                    print(output_from_sysiolog)
                    print("not eq!\n")
                    exit(-1)
        print("proc: ", len(proc_forked))
        print("proc_killed: ", len(proc_killed))
        print("proc_error: ", len(proc_error))


    def buildChildProcOutputFromFile(self, case_dir, proc):
        output_dict = {}

        file_prefix = "mut_output-" + str(proc) + "-"
        for item in os.listdir(case_dir):
            if item.startswith(file_prefix):
                file_name = item
                file_content = None
                with open(os.path.join(case_dir, file_name), 'rb') as file:
                    file_content = file.read()
                file_name = file_name.split(file_prefix)[1];
                output_dict[file_name] = file_content
        return ChildProcOutput(output_dict)
    
    def resolveWriteCall(self, raw_call):
        # {
        # INDEX: 0
        # TYPE: write
        # CONTENT: 
        # WriteCall : stdout : output begin!
        # 23
        # }
        target_file = None
        content = bytes()

        # print(raw_call)
        raw_call.pop()
        flag = 0
        for l in raw_call:
            if l.startswith("TARGET FILE:"):
                target_file = l.split("TARGET FILE:")[1].strip()
                continue
            
            if l.startswith("CONTENT:"):
                assert flag == 0
                flag = 1
                continue
            
            if flag:
                content = content + l.encode('utf-8')
        return WriteCall(target_file, content)

    def appendWrite(self, output_dict, write_call: WriteCall):
        if write_call.target_file not in output_dict:
            output_dict[write_call.target_file] = bytes()
        output_dict[write_call.target_file] += write_call.content

    # def fillFilesToOutputDict(self, output_dict, case_dir):
    #     # 无奈之举，纯粹是为了和mut tool接轨
    #     for item in os.listdir(case_dir):
    #         if item.startswith("mut_output-0-"):
    #             filename = item.split("mut_output-0-")[1]
    #             output_dict[filename] = bytes()

    def buildChildProcOutputFromSYSIOLog(self, case_dir, proc):
        output_dict = {}
        # self.fillFilesToOutputDict(output_dict, case_dir)
        log_file = "log_sysio-" + str(proc)
        with open(os.path.join(case_dir, log_file), "rb",) as f:
            # 直接以文本文件方式打开，可能存在编码错误，如果选择error='ignore'，又可能会导致一些信息丢失。
            # raw_call = []
            # flag = 0
            # f = f.split(b'CONTENT: \n')
            # for l in f:
            #     # l = l.strip()
            #     if l.startswith("{SYSIO_LOG_BEGIN"):
            #         flag = 1
            #     elif l.startswith("}SYSIO_LOG_END"):
            #         write_call = self.resolveWriteCall(raw_call)
            #         self.appendWrite(output_dict, write_call)
            #         raw_call = []
            #         flag = 0
            #     if flag:
            #         raw_call.append(l)

            data = f.read()
            data = data.split(b'[KILLED]')[0].split(b'TARGET FILE: ')
            for l in data:
                if len(l.split(b'\nCONTENT:')) == 1:
                    continue
                # print(l)
                target_file = l.split(b'\nCONTENT:')[0].decode()
                content_prepare = l.split(b'CONTENT: \n')
                assert len(content_prepare) == 2
                content = content_prepare[1].split(b'\n}SYSIO_LOG_END')[0]

                if target_file not in output_dict:
                    output_dict[target_file] = bytes()
                output_dict[target_file] += content

        return ChildProcOutput(output_dict)
                



    def readProcNeedCheck(self, forked_path):
        proc = []
        proc_killed = []
        proc_error = []
        proc_same_output = []
        with open(forked_path) as f:
            for l in f:
                if l.startswith('++') or l.startswith('--'):
                    continue
                else:
                    l = l.strip()
                    pattern_r42 = r'(\d+)=>(\d+):(\d+):(\d+)\((.+)\): (\d+)/r42'
                    pattern_r233 = r'(\d+)=>(\d+):(\d+):(\d+)\((.+)\): (\d+)/r233'
                    pattern_mut_id = r'(\d+)=>(\d+):(\d+):(\d+)(.+)'
                    match_r42 = re.search(pattern_r42, l)
                    match_r233 = re.search(pattern_r233, l)
                    match_mut_id = re.search(pattern_mut_id, l)
                    if match_mut_id:
                        proc.append(match_mut_id.group(4))
                    if match_r42:
                        proc_killed.append(match_r42.group(4))
                    if match_r233:
                        proc_error.append(match_r233.group(4))
        return proc, proc_killed, proc_error
                



    def readCaseInRun(self, run_log_dir):
        case_in_run = {}
        items = os.listdir(run_log_dir)
        items = sorted(items, key=lambda x: int(x.split('_')[1]))
        for item in items:
            item_path = os.path.join(run_log_dir, item)
            assert os.path.isdir(item_path)
            case_in_run[item] = item_path
        return case_in_run


if __name__ == '__main__':
    try:
        run_log_dir = sys.argv[1]
        if len(sys.argv) >= 3:
            specified_case = sys.argv[2]
            specified_proc = []
            for proc in sys.argv[3:]:
                specified_proc.append(int(proc))
            run = Run(run_log_dir, specified_case, specified_proc)
        else:
            run = Run(run_log_dir)

        print(run)
    except Exception as e:
        traceback.print_exc(file=sys.stdout)