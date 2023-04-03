import PySimpleGUI as sg
import os
import cv2
import numpy as np
import time
import sys
import os
import threading

py_file = os.path.dirname(os.path.realpath(sys.argv[0]))
times = 0
video_path=''

def code(maxFrame, useHamming,fps):
    dir_path = ''
    dir_path = sg.popup_get_file("Please select the file")
    if not dir_path:
        return
    if (not os.path.exists(dir_path)):
        return
    if os.path.exists(py_file + "\\in.mp4"):
        os.remove(py_file + "\\in.mp4")
    print(py_file + "\\in.mp4")
    cmd = 'encoder ' + dir_path + ' in.mp4 ' + str(maxFrame) + useHamming+' '+fps
    sg.popup_quick_message('Encoding...', background_color='white')
    os.chdir(py_file)
    os.system(cmd)
    print(cmd)
    video_path=py_file + "\\in.mp4"
    video = cv2.VideoCapture(py_file + "\\in.mp4")
    times = video.get(7) / video.get(5)
    print(times)
    confirm_play_video()
    return video_path


def confirm_play_video():
    print("confirming")
    layout1 = [[sg.Text('Do you need to play the video?')],
               [sg.Button('Yes', size=(11, 2)), sg.Button('No', size=(11, 2))]]

    window2 = sg.Window('Finished', layout1, size=(225, 80))
    while True:
        event2, values2 = window2.read()
        if event2 == sg.WINDOW_CLOSED or event2 == 'No':
            break
        elif event2 == 'Yes':
            print("Yes")
            window2.close()
            play_video(py_file + "\\in.mp4")
            break
    window2.close()


def play_video(file_path):
    print(file_path)
    if not os.path.exists(file_path):
        layout1 = [[sg.Text('Still not have video\nPlease encode first')]]
        window2 = sg.Window('Error', layout1, size=(225, 80))
        while True:
            event2, values2 = window2.read()
            if event2 == sg.WINDOW_CLOSED or event2 == 'No':
                break
        window2.close()
        return
    print(file_path)
    video = cv2.VideoCapture(file_path)
    frame_count = int(video.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = int(video.get(cv2.CAP_PROP_FPS))
    window_width = int(video.get(cv2.CAP_PROP_FRAME_WIDTH))
    window_height = int(video.get(cv2.CAP_PROP_FRAME_HEIGHT))
    cv2.namedWindow("Video", cv2.WINDOW_NORMAL)
    cv2.resizeWindow("Video", 1920, 1080)
    cv2.moveWindow("Video", 0, 0)
    times = video.get(7) / video.get(5)
    print(times)
    # Set up the progress bar parameters.
    bar_width = int(window_width)
    bar_height = 30
    bar_x = int((window_width - bar_width) / 2)
    bar_y = int(window_height - bar_height)
    progress = 0

    def mouse_callback(event3, x, y, flags, param):
        nonlocal progress
        if event3 == cv2.EVENT_LBUTTONDOWN:
            if bar_x <= x <= bar_x + bar_width and bar_y + 30 <= y <= bar_y + bar_height + 30:
                progress = 0
                video.set(cv2.CAP_PROP_POS_FRAMES, int(progress * frame_count))

    cv2.setMouseCallback("Video", mouse_callback)

    while True:
        ret, frame = video.read()

        if ret:
            current_frame = int(video.get(cv2.CAP_PROP_POS_FRAMES))
            progress = current_frame / frame_count

            bar_image = 0 * np.ones((bar_height, bar_width, 3), np.uint8)
            cv2.rectangle(bar_image, (0, 0), (int(bar_width * progress), bar_height), (0, 255, 255), -1)

            combined_image = np.vstack((frame, 0 * np.ones((30, window_width, 3), np.uint8)))
            combined_image[window_height + 30 - bar_height:window_height + 30, bar_x:bar_x + bar_width] = bar_image

            cv2.imshow("Video", combined_image)

        key = cv2.waitKey(1) & 0xFF
        if key == 27 or cv2.getWindowProperty("Video", cv2.WND_PROP_VISIBLE) < 1:
            break
    video.release()
    cv2.destroyAllWindows()


def decode_video(useHamming):
    if not useHamming:
        layout1 = [[sg.Text('Please encode first')]]
        window2 = sg.Window('Error', layout1, size=(225, 80))
        while True:
            event2, values2 = window2.read()
            if event2 == sg.WINDOW_CLOSED or event2 == 'No':
                break
        window2.close()
        return
    print('----decode-----')
    dir_path_in = sg.popup_get_file('Please select the video')
    if not dir_path_in:
        return
    if (not os.path.exists(dir_path_in)):
        return
    video = cv2.VideoCapture(dir_path_in)
    times = video.get(7) / video.get(5)
    os.chdir(py_file)
    cmd = 'decoder ' + dir_path_in + ' out.bin '+ useHamming
    sg.popup_quick_message('Decoding...', background_color='white')
    print(cmd)
    os.system(cmd)
    fp = open("./res.txt")
    res = fp.read()
    outsize = os.path.getsize('./out.bin')
    print(outsize)
    rate = 0
    res+="VideoTime:"+str(round(times,2))+'s\n'
    if times != 0:
        rate = round(outsize / times * 8 / 1024,2)

    if rate != 0:
        res += "Rate:" + str(rate) + "kb/s"
    print(outsize)
    print(times)
    print(res)
    layout2 = [[sg.Text(res)],
               [sg.Button('OK')]]
    window3 = sg.Window('Finished', layout2, size=(225, 100))
    while True:
        event3, values3 = window3.read()
        if event3 == sg.WINDOW_CLOSED or event3 == 'OK':
            break


if __name__ == '__main__':
    print(py_file)
    encode = sg.Button('Encode', size=(39, 1))
    decode = sg.Button('Decode', size=(39, 1))
    in_put = sg.Input(size=(10, 5), key='In',default_text='5000')
    play = sg.Button('PlayVideo', size=(39, 1))
    text = sg.Text('  MaxVideoTime(ms)', size=(18, 1))
    fpslist=[10,11,12,13,14,15,16,17,18,19,20]
    comb=sg.Combo(fpslist,key='comb',default_value='FPS',size=(15,1))
    check = sg.Checkbox('UseHammingCode', size=(15, 1), key='use')
    layout = [
        [text, in_put],
        [check,comb],
        [encode],
        [play],
        [decode]
    ]
    sg.theme('LightBlue2')
    window = sg.Window('LightTransmission', layout, size=(265, 165))
    vtime = 0
    video_path=''
    useHamming=''
    while True:
        event, values = window.read()
        if event == 'Encode':
            if values['In']:
                maxFrame = values['In']
            else:
                maxFrame = 1000000
            if values['use']:
                useHamming = ' 1'
            else:
                useHamming = ' 0'
            if values['comb']=='FPS':
                fps=15
            else:
                fps=values['comb']
            print(maxFrame)
            print(useHamming)
            video_path=code(maxFrame, useHamming,str(fps))
        elif event == sg.WINDOW_CLOSED:
            break
        elif event == 'Decode':
            decode_video(useHamming)
        elif event == 'PlayVideo':
            play_video(video_path)
