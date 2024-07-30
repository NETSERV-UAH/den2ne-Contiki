from graphics import *
import json
import numpy as np
import pdb

#a941 => 30 (Root)
#3163 => 98 (Common)
#0245 => 85 (Common)
#9193 => 10 (Common)

def json_file(file):
    f = open(file)
    data = json.load(f)
    f.close()
    return data

def draw_next(win):
    button = Rectangle(Point(1130,840), Point(1280,880))
    button.setFill("green")
    button.draw(win)
    text = Text(button.getCenter(), "Press 'n' to continue")
    text.draw(win)

def create_arrow(origin, destiny, mes_type, flag):                              #FLAG TO AVOID CONFUSING ARROW DIRECTIONS
    if abs(origin.getCenter().getX() - destiny.getCenter().getX()) > abs(origin.getCenter().getY() - destiny.getCenter().getY()):
        if origin.getCenter().getX() - destiny.getCenter().getX() > 0:                  #ORIGIN AT DESTINY'S LEFT
            point_origin = Point(origin.getP1().getX(),
                                 origin.getP1().getY()+origin.getRadius()-flag*4)       #LEFT
            point_destiny = Point(destiny.getP2().getX(),
                                  destiny.getP2().getY()-destiny.getRadius()-flag*4)    #RIGHT
        else:                                                                           #ORIGIN AT DESTINY'S RIGHT
            point_origin = Point(origin.getP2().getX(),
                                 origin.getP2().getY()-origin.getRadius()+flag*4)       #RIGHT
            point_destiny = Point(destiny.getP1().getX(),
                                  destiny.getP1().getY()+destiny.getRadius()+flag*4)    #LEFT
    else:
        if origin.getCenter().getY() - destiny.getCenter().getY() > 0:                  #ORIGIN BELOW DESTINY
            point_origin = Point(origin.getP1().getX()+origin.getRadius(),#-flag*4,
                                 origin.getP1().getY())                                 #UP
            point_destiny = Point(destiny.getP2().getX()-destiny.getRadius(),#-flag*4,
                                  destiny.getP2().getY())                               #DOWN
        else:                                                                           #ORIGIN ABOVE DESTINY
            point_origin = Point(origin.getP2().getX()-origin.getRadius(),#+flag*4,
                                 origin.getP2().getY())                                 #DOWN
            point_destiny = Point(destiny.getP1().getX()+destiny.getRadius(),#+flag*4,
                                  destiny.getP1().getY())                               #UP
        
    arrow = Line(point_origin, point_destiny)
    if (mes_type == "Hello"):
        arrow.setFill("red")
    elif (mes_type == "SetHLMAC"):
        arrow.setFill("green")
    elif (mes_type == "Load"):
        arrow.setFill("blue")
    elif (mes_type == "Share"):
        arrow.setFill("orange")
    arrow.setArrow("last")
    return arrow

def arrow_text(arrow, message, mac):
    if abs(arrow.getP1().getX() - arrow.getP2().getX()) > abs(arrow.getP1().getY() - arrow.getP2().getY()):
        if arrow.getP1().getX() - arrow.getP2().getX() > 0:                             #FROM LEFT TO RIGHT
            desfaseX = 30
            desfaseY = -15
        else:                                                                           #FROM RIGHT TO LEFT
            desfaseX = 30
            desfaseY = +15
    else:
        if arrow.getP1().getY() - arrow.getP2().getY() > 0:                             #UPWARDS
            if (message['Type'] == "SetHLMAC"):
                desfaseX = 60
                desfaseY = 15
            else:
                desfaseX = 20
                desfaseY = 0
        else:                                                                           #DOWNWARDS
            if (message['Type'] == "SetHLMAC"):
                desfaseX = -60
                desfaseY = -15
            else:
                desfaseX = -20
                desfaseY = 0
    if (message['Type'] == "Hello"):
        # return Text(Point(arrow.getCenter().getX()- 2*abs(desfaseX), arrow.getCenter().getY()-abs(desfaseY)), message['Type'])
        return Text(Point(arrow.getCenter().getX()- 2*abs(desfaseX), arrow.getCenter().getY()-abs(desfaseY)),"")
    elif (message['Type'] == "SetHLMAC"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message['Content']['Prefix'] + '.' + message['Content']['HLMAC'][mac])
    elif (message['Type'] == "Load"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message["Value"])
    elif (message['Type'] == "Share"):
        return Text(Point(arrow.getCenter().getX()+desfaseX, arrow.getCenter().getY()+desfaseY), message['Type'] + ": " + message["Value"])

def show_table(win, num_nodes):
    rec = Rectangle(Point(110,  10), Point(290, 40)).draw(win)
    rec.setFill("light blue")
    rec.setWidth(3)
    Text(Point(200, 28), "HLMAC").draw(win).setSize(20)
    rec2 = Rectangle(Point(290, 10), Point(390, 40)).draw(win)
    rec2.setFill("light blue")
    rec2.setWidth(3)
    Text(Point(340, 28), "LOAD").draw(win).setSize(20)
    rec3 = Rectangle(Point(390, 10), Point(490, 40)).draw(win)
    rec3.setFill("light blue")
    rec3.setWidth(3)
    Text(Point(440, 28), "SHARE").draw(win).setSize(20)

def show_node_table(win, number):
    rec = Rectangle(Point(10, 40+50*(number-1)), Point(110, 40+50*number)).draw(win)
    rec.setFill("light blue")
    rec.setWidth(3)
    Text(Point(60, 20+50*number), "Node " + str(number)).draw(win).setSize(22)
    Rectangle(Point(110, 40+50*(number-1)), Point(290, 40+50*number)).draw(win).setWidth(3)
    Rectangle(Point(290, 40+50*(number-1)), Point(390, 40+50*number)).draw(win).setWidth(3)
    Rectangle(Point(390, 40+50*(number-1)), Point(490, 40+50*number)).draw(win).setWidth(3)

def draw_nodes(win, dictionary):
    for val in dictionary.values():
        val["Circle"].draw(win).setWidth(3)
        val["Text"].draw(win).setSize(20)

def draw_node_info(win, node):
    node["HLMAC"].draw(win).setSize(24)
    node["LOAD"].draw(win).setSize(24)
    node["SHARE"].draw(win).setSize(24)

def erase_nodes(dictionary):
    for val in dictionary.values():
        val["Circle"].undraw()
        val["Text"].undraw()

def node_conn(win, data, time, dictionary):

    #NUMBER OF STEPS IN TOPOLOGY
    len_hlmac_ini = 2
    len_hlmac_step = 3
    max_step = int(0)
    for k in range(len(data['Node'])):
        for c in range(len(data['Node'][k]['Message'])):
            if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time):
                if int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1) > max_step:
                    max_step = int((len(data['Node'][k]['Message'][c]['Value']) - len_hlmac_ini)/len_hlmac_step + 1)
    hlmac_len_list = [[] for i in range(max_step)]

    added_nodes = []
    hlmac_list = [[] for i in range(max_step)]
    for l in range(max_step):
        if l == 0:
            for k in range(len(data['Node'])):
                for c in range(len(data['Node'][k]['Message'])):
                    if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time) and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes:
                        hlmac_len_list[l].append(k)
                        hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                        added_nodes.append(k)
        else:
            for i in range(len(hlmac_len_list[l-1])):
                for k in range(len(data['Node'])):
                    for c in range(len(data['Node'][k]['Message'])):
                        if data['Node'][k]['Message'][c]['Type'] == "INFO" and data['Node'][k]['Message'][c]['Content'] == "HLMAC added" and data['Node'][k]['Message'][c]['Timestamp'] == str(time) and len(data['Node'][k]['Message'][c]['Value']) == len_hlmac_ini + len_hlmac_step * l and not k in added_nodes and hlmac_list[l-1][i] in data['Node'][k]['Message'][c]['Value']:
                            hlmac_len_list[l].append(k)
                            hlmac_list[l].append(data['Node'][k]['Message'][c]['Value'])
                            added_nodes.append(k)

    #ASSIGNS POSITION IN DRAWING ACCORDING TO HLMAC LENGTH
    erase_nodes(dictionary)
    for c in range(len(hlmac_len_list)):
        if c == 0: #or len(hlmac_len_list[c]) <= 2:
            y_offset = 0
        else:
            y_offset = 35
        x_offset = ((len(hlmac_len_list[c]) / 2) - 1) * 200
        for k in range(len(hlmac_len_list[c])):
            x_offset -= 200
            y_offset = -y_offset
            if not data['Node'][hlmac_len_list[c][k]]['MAC'] in dictionary:
                dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']] = {"Circle": Circle(Point(500-x_offset,50+y_offset+220*c), 30), "Text": Text(Circle(Point(500-x_offset,50+y_offset+220*c), 30).getCenter(), str(len(dictionary.keys())+1))}
                dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']].update({"HLMAC": Text(Point(200, 70+50*(len(dictionary.keys())-1)), "-"), "LOAD": Text(Point(340, 70+50*(len(dictionary.keys())-1)), "-"), "SHARE": Text(Point(440, 70+50*(len(dictionary.keys())-1)), "-")})
                show_node_table(win, len(dictionary.keys()))
                draw_node_info(win, dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']])
            else:
                dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']].update({"Circle": Circle(Point(500-x_offset,50+y_offset+220*c), 30), "Text": Text(Circle(Point(500-x_offset,50+y_offset+220*c), 30).getCenter(), dictionary[data['Node'][hlmac_len_list[c][k]]['MAC']]["Text"].getText())})
    return dictionary
    

def main():
    message_types = ["Hello", "SetHLMAC", "Load", "Share"]
    message_colors =  ["red", "green", "blue", "orange"]
    win = GraphWin("First Escenary", 1300, 900)
    win.setBackground("white")

    Rectangle(Point(1100,  40), Point(1130,  70)).draw(win).setFill("red")
    Rectangle(Point(1100,  90), Point(1130, 120)).draw(win).setFill("green")
    Rectangle(Point(1100, 140), Point(1130, 170)).draw(win).setFill("blue")
    Rectangle(Point(1100, 190), Point(1130, 220)).draw(win).setFill("orange")

    Text(Point(1170, 60), "Hello").draw(win).setSize(20)
    Text(Point(1205, 110), "SetHLMAC").draw(win).setSize(20)
    Text(Point(1170, 160), "Load").draw(win).setSize(20)
    Text(Point(1175, 210), "Share").draw(win).setSize(20)

    data = json_file("no_timers/demo_4_mesh.json")
    #data = json_file("timers/test_demo_7.json")
    #data = json_file("prueba_demo_4_mesh.json")

    dictionary = {}
    show_table(win, len(data['Node']))

    #NUMBER OF ITERATIONS
    for k in range(len(data['Node'])):
        if data['Node'][k]['Type'] == "Root":
            for c in range(len(data['Node'][k]['Message'])):
                if data['Node'][k]['Message'][c]['Type'] == "SetHLMAC":
                    timestamps = int(data['Node'][k]['Message'][c]['Content']['Timestamp'])

    first_timestamp = []
    for k in range(len(data['Node'])):
        for c in range(len(data['Node'][k]['Message'])):
            if data['Node'][k]['Message'][c]['Type'] == "SetHLMAC":
                first_timestamp.append(int(data['Node'][k]['Message'][c]['Content']['Timestamp']))
                break

    last_message = np.zeros(len(data['Node']), dtype=int)
    arrow = []
    arrow_message = []
    for time in range(timestamps+1):
        #NODE CONTAINS EACH NODE'S CIRCLE AND NODE_INDEX ITS NUMBER
        dictionary = node_conn(win, data, time, dictionary)

        draw_nodes(win, dictionary)
        key = win.getKey()
        while key != "n":
            key = win.getKey()

        edge_nodes = []
        for c in message_types:
            if c == "Hello":
                for k in range(len(data['Node'])):
                    while first_timestamp[k] <= time and last_message[k] < len(data['Node'][k]['Message']) and data['Node'][k]['Message'][last_message[k]]['Type'] != "SetHLMAC": #(data['Node'][k]['Message'][last_message[k]]['Type'] == "Hello" or data['Node'][k]['Message'][last_message[k]]['Type'] == "INFO"):
                        if data['Node'][k]['MAC'] != data['Node'][k]['Message'][last_message[k]]['Origin'] and data['Node'][k]['Message'][last_message[k]]['Type'] == 'Hello':
                            arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][last_message[k]]['Origin']]["Circle"], dictionary[data['Node'][k]['MAC']]["Circle"], data['Node'][k]['Message'][last_message[k]]['Type'], 0))
                            arrow[len(arrow)-1].draw(win).setWidth(3)
                            arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][last_message[k]], data['Node'][k]['MAC']))
                            arrow_message[len(arrow)-1].draw(win).setSize(20)
                        elif data['Node'][k]['Message'][last_message[k]]['Type'] == "INFO":
                            if data['Node'][k]['Message'][last_message[k]]['Content'] == "Edge":
                                if not data['Node'][k]['MAC'] in edge_nodes:
                                    edge_nodes.append(data['Node'][k]['MAC'])
                            elif data['Node'][k]['Message'][last_message[k]]['Content'] == "HLMAC added":
                                dictionary[data['Node'][k]['MAC']]["HLMAC"].setText(data['Node'][k]['Message'][last_message[k]]['Value'])
                        last_message[k] += 1
                key = win.getKey()
                while key != "n":
                    key = win.getKey()
                for i in range(len(arrow)):
                    arrow[i].undraw()
                    arrow_message[i].undraw()
            elif c == "SetHLMAC":
                prefix = 2
                sethlmac_done = 0
                while sethlmac_done == 0:
                    sethlmac_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        assigned_nodes = []
                        while first_timestamp[k] <= time and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "SetHLMAC" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'SetHLMAC' and len(data['Node'][k]['Message'][aux_last_message[k]]['Content']['Prefix']) == prefix:
                                sethlmac_done = 0
                                assigned_nodes.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']]["Circle"], dictionary[data['Node'][k]['MAC']]["Circle"], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":                                    
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added" and data['Node'][k]['MAC'] in assigned_nodes:
                                    dictionary[data['Node'][k]['MAC']]["HLMAC"].setText(data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                            aux_last_message[k] += 1
                    if sethlmac_done == 0:
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()
                    prefix += 3
                last_message = aux_last_message.copy()

            elif c == "Load":
                load_nodes_prev  = edge_nodes.copy()
                load_nodes  = edge_nodes.copy()
                load_nodes_next  = []
                load_done = 0
                while load_done == 0:
                    load_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        while first_timestamp[k] <= time and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Load" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Share" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Load' and data['Node'][k]['Message'][aux_last_message[k]]['Origin'] in load_nodes:
                                load_done = 0
                                if not data['Node'][k]['MAC'] in load_nodes_prev and not data['Node'][k]['MAC'] in load_nodes_next and aux_last_message[k]+1 < len(data['Node'][k]['Message']) and data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]+1]['Origin']:
                                    load_nodes_next.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']]["Circle"], dictionary[data['Node'][k]['MAC']]["Circle"], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Load' and data['Node'][k]['MAC'] in load_nodes:
                                dictionary[data['Node'][k]['MAC']]["LOAD"].setText(data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    dictionary[data['Node'][k]['MAC']]["HLMAC"].setText(data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                            aux_last_message[k] += 1
                    load_nodes_prev.extend(load_nodes)
                    load_nodes = load_nodes_next.copy()
                    load_nodes_next.clear()
                    if load_done == 0:
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()

            elif c == "Share":
                share_nodes_prev  = edge_nodes.copy()
                share_nodes  = edge_nodes.copy()
                share_nodes_next  = []
                share_done = 0
                while share_done == 0:
                    share_done = 1
                    aux_last_message = last_message.copy()
                    for k in range(len(data['Node'])):
                        while first_timestamp[k] <= i and aux_last_message[k] < len(data['Node'][k]['Message']) and (data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Load" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "Share" or data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO"):
                            if data['Node'][k]['MAC'] != data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Share' and data['Node'][k]['Message'][aux_last_message[k]]['Origin'] in share_nodes:
                                share_done = 0
                                if not data['Node'][k]['MAC'] in share_nodes_prev and not data['Node'][k]['MAC'] in share_nodes_next and aux_last_message[k]+1 < len(data['Node'][k]['Message']) and data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]+1]['Origin']:
                                    share_nodes_next.append(data['Node'][k]['MAC'])
                                arrow.append(create_arrow(dictionary[data['Node'][k]['Message'][aux_last_message[k]]['Origin']]["Circle"], dictionary[data['Node'][k]['MAC']]["Circle"], data['Node'][k]['Message'][aux_last_message[k]]['Type'], 1))
                                arrow[len(arrow)-1].draw(win)
                                arrow_message.append(arrow_text(arrow[len(arrow)-1], data['Node'][k]['Message'][aux_last_message[k]], data['Node'][k]['MAC']))
                                arrow_message[len(arrow)-1].draw(win)
                            elif data['Node'][k]['MAC'] == data['Node'][k]['Message'][aux_last_message[k]]['Origin'] and data['Node'][k]['Message'][aux_last_message[k]]['Type'] == 'Share' and data['Node'][k]['MAC'] in share_nodes:
                                dictionary[data['Node'][k]['MAC']]["SHARE"].setText(data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                            elif data['Node'][k]['Message'][aux_last_message[k]]['Type'] == "INFO":
                                if data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Edge":
                                    if not data['Node'][k]['MAC'] in edge_nodes:
                                        edge_nodes.append(data['Node'][k]['MAC'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "HLMAC added":
                                    dictionary[data['Node'][k]['MAC']]["LOAD"].setText(data['Node'][k]['Message'][aux_last_message[k]]['Value'])
                                elif data['Node'][k]['Message'][aux_last_message[k]]['Content'] == "Convergence":
                                    convergence_time = float("{:.2f}".format(float(data['Node'][k]['Message'][aux_last_message[k]]['Value'])/128))
                            aux_last_message[k] += 1
                    share_nodes_prev.extend(share_nodes)
                    share_nodes = share_nodes_next.copy()
                    share_nodes_next.clear()
                    if share_done == 0:
                        if convergence_time != 0:
                            if time == 0:
                                convergence_text = Text(Point(230, 80+50*len(data['Node'])), "Convergence time: " + str(convergence_time) + "s")
                                convergence_text.setSize(30)
                                convergence_text.draw(win)
                            else:
                                convergence_text.undraw()
                                convergence_text = Text(Point(230, 80+50*len(data['Node'])), "Convergence time: " + str(convergence_time) + "s")
                                convergence_text.setSize(30)
                                convergence_text.draw(win)
                            convergence_time = 0
                        key = win.getKey()
                        while key != "n":
                            key = win.getKey()
                    for i in range(len(arrow)):
                        arrow[i].undraw()
                        arrow_message[i].undraw()
                last_message = aux_last_message.copy()
                edge_nodes.clear()

    win.close()

main()