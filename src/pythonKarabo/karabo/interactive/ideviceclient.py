from IPython import start_ipython


def main():
    code_to_run = 'from karabo.interactive.deviceClient import *'
    start_ipython(code_to_run=code_to_run, force_interact=True,
                  display_banner=False)

if __name__ == '__main__':
    main()
