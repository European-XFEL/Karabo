@echo off 
python -m pytest -v --disable-warnings --pyargs --junitxml=junit.karabo.common.xml karabo.common && (
    python -m pytest -v --disable-warnings --pyargs --junitxml=junit.karabo.native.xml karabo.native && (
        python -m pytest -v --disable-warnings --pyargs --junitxml=junit.karabogui.xml karabogui
    )
) || EXIT 1