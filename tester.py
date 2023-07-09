import shutil
import subprocess

task_list = ['pingpong-tasks3', 'pingpong-tasks2', 'pingpong-tasks1', 'pingpong-dispatcher', 'pingpong-scheduler']
shutil.copyfile('Makefile', 'backup_Makefile')
og_mf = open('Makefile', 'r').readlines()


def change_compile(task):
    new_mf = []
    for line in og_mf:
        if 'OBJ' in line and '#' not in line:
            for name_task in task_list:
                if name_task in line:
                    line = line.replace(name_task, task)
        new_mf.append(line)

    return new_mf


def test_task(task):
    with open('Makefile', 'w') as mf:
        mf.write(''.join(change_compile(task)))

    subprocess.run('make')
    output = subprocess.check_output('./ppos')
    open('test', 'w').write(output.decode('utf-8'))

    subprocess.run(['diff', task.replace('pingpong-', '') + '.txt', 'test'])


for task in task_list:
    test_task(task)

shutil.move('backup_Makefile', 'Makefile')
