import React, { Component } from 'react';
import {
  AppRegistry,
  StyleSheet,
  Text,
  View,
  TouchableHighlight,
  NativeAppEventEmitter,
  NativeEventEmitter,
  NativeModules,
  Platform,
  PermissionsAndroid,
  ListView,
  ScrollView,
  Button,
} from 'react-native';
import Dimensions from 'Dimensions';
import BleManager from 'react-native-ble-manager';
import TimerMixin from 'react-timer-mixin';
import reactMixin from 'react-mixin';

const window = Dimensions.get('window');
const ds = new ListView.DataSource({rowHasChanged: (r1, r2) => r1 !== r2});

const BleManagerModule = NativeModules.BleManager;
const bleManagerEmitter = new NativeEventEmitter(BleManagerModule);

const serviceId = 'cb73af71-04cf-4938-99df-756fb8f7cbb9'
const charId = '8573d3e8-c84f-4efe-8cbf-0dd72b7ac1f7'

const weights =  {
  speed_bumps: 0.5,
  fast_accls: 1.5,
  fast_stops: 1.5,
  sharp_turns: 1.5,
  shocks: 0.1,
}


export default class App extends Component {
  constructor(){
    super()

    this.state = {
      connecting: false,
    }

    this.handleDiscoverPeripheral = this.handleDiscoverPeripheral.bind(this);
    this.handleStopScan = this.handleStopScan.bind(this);
    this.handleDisconnectedPeripheral = this.handleDisconnectedPeripheral.bind(this);
  }

  componentDidMount() {
    BleManager.start({showAlert: false, allowDuplicates: false});

    this.handlerDiscover = bleManagerEmitter.addListener('BleManagerDiscoverPeripheral', this.handleDiscoverPeripheral );
    this.handlerStop = bleManagerEmitter.addListener('BleManagerStopScan', this.handleStopScan );
    this.handlerDisconnect = bleManagerEmitter.addListener('BleManagerDisconnectPeripheral', this.handleDisconnectedPeripheral );

    if (Platform.OS === 'android' && Platform.Version >= 23) {
        PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION).then((result) => {
            if (result) {
              console.log("Permission is OK");
            } else {
              PermissionsAndroid.requestPermission(PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION).then((result) => {
                if (result) {
                  console.log("User accept");
                } else {
                  console.log("User refuse");
                }
              });
            }
      });
    }
  }

  componentWillUnmount() {
    this.handlerDiscover.remove();
    this.handlerStop.remove();
    this.handlerDisconnect.remove();
    this.handlerUpdate.update();
  }

  handleDisconnectedPeripheral(data) {
    console.log('Disconnected from ' + data.peripheral);
  }

  handleStopScan() {
    this.setState({connecting: false})
  }

  connect() {
    this.setState({connecting: true})

    BleManager.scan([], 10, true).then((results) => {
      console.log('Scanning...');
    });
  }

  handleDiscoverPeripheral(peripheral){
    if (peripheral.name == 'BB') {
      this.connectToDevice(peripheral)
    }
  }

  disconnect(peripheral) {
    BleManager.disconnect(this.state.deviceId)
    .then(() => {
      console.log('disconnected');
      this.setState({deviceId: undefined, bb_state: undefined})
    })
    .catch((error) => {
      console.log('error disconnecting', error)
    })
  }


  connectToDevice(peripheral) {
    BleManager.connect(peripheral.id).then(() => {
      console.log('Connected to ' + peripheral.id);

      BleManager.retrieveServices( peripheral.id )
      .then((peripherralInfo) => {
        this.setState({deviceId: peripheral.id})
        this.setState({connecting: false})

        this.getState()
      })
      .catch((error) => {
        console.log('error retrieveServices', error)
        this.setState({connecting: false})
      })

    }).catch((error) => {
      this.setState({connecting: false})
      console.log('Connection error', error);
    });
  }

  reset () {
    BleManager.writeWithoutResponse(this.state.deviceId, serviceId, charId, [2])
    .then((data) => {
      console.log('sent reset');
    })
  }

  getState() {
    BleManager.writeWithoutResponse(this.state.deviceId, serviceId, charId, [1])
    .then((data) => {
      this.setTimeout(() => {
        BleManager.read(this.state.deviceId, serviceId, charId)
        .then((data) => {
          this.setState({
            bb_state: {
              speed_bumps: data[0],
              fast_accls: data[1],
              fast_stops: data[2],
              sharp_turns: data[3],
              shocks: data[4],
            }
          })
        })
        .catch((error) => {
          console.log('error reading', error )
        });
      }, 500)
    })
    .catch((error) => {
      console.log('error writing', error )
    });
  }

  render() {
    let connectButtonString = 'Connect'
    let connectButtonEnabled = false

    if (this.state.deviceId) {
      connectButtonString = 'Connected'
    }
    else if (this.state.connecting) {
      connectButtonString = 'Connecting'
    }
    else {
      connectButtonEnabled = true
    }

    return (
      <View>
        <Button
          title={connectButtonString}
          enabled={connectButtonEnabled}
          onPress={() => this.connect()}
        />
        {this.state.bb_state &&
          <BBState bb_state={this.state.bb_state} />
        }
        {this.state.deviceId &&
          <Button title='Reset' onPress={() => this.reset()} />
        }

        {this.state.deviceId &&
          <Button title='Disconnect' onPress={() => this.disconnect()} />
        }
      </View>
    );
  }
}


class BBState extends Component {
  render() {
    const s = this.props.bb_state

    const score = _calcScore(weights, s)
    const w = weights

    return (
      <View style={styles.bb_state}>
        <Box title="Speed Bumps" number={s.speed_bumps} score={s.speed_bumps * w.speed_bumps}/>
        <Box title="Fast Accls" number={s.fast_accls} score={s.fast_accls * w.fast_accls}/>
        <Box title="Fast Stops" number={s.fast_stops} score={s.fast_stops * w.fast_stops}/>
        <Box title="Sharp Turns" number={s.sharp_turns} score={s.sharp_turns * w.sharp_turns}/>
        <Box title="Shocks" number={s.shocks} score={s.shocks * w.shocks}/>
        <Box title="Score" number={score} score={score} textStyle={{color: 'white', fontWeight: 'bold'}} />
      </View>
    )
  }
}

function _calcScore(weights, state) {
  return Object.keys(state).reduce((acc, k) => {
    acc += weights[k] * state[k]
    return acc
  }, 0)
}


class Box extends Component {
  render() {
    const title = this.props.title
    const number = this.props.number
    const score = this.props.score
    const red = '#e91e63'
    const yellow = '#ffeb3b'
    const green = '#8bc34a'

    if (score > 100)
      color = red
    else if (score > 30)
      color = yellow
    else
      color = green

    return (
      <View style={[styles.box, {backgroundColor: color}]}>
        <Text style={[this.props.textStyle, styles.title]}>{title}</Text>
        <Text style={[this.props.textStyle, styles.number]}>{number}</Text>
      </View>
    )
  }
}


reactMixin(App.prototype, TimerMixin);

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#FFF',
    width: window.width,
    height: window.height
  },
  
  bb_state: {
    flexDirection: 'row',
    flexWrap: 'wrap'
  },
  
  box: {
    borderWidth: 0.5,
    width: window.width/ 2,
    height: 180,
    alignItems: 'center',
    justifyContent: 'center',
  },

  number: {
    fontSize: 50,
  },

  title: {
    fontSize: 15,
  },

  scroll: {
    flex: 1,
    backgroundColor: '#f0f0f0',
    margin: 10,
  },
  row: {
    margin: 10
  },
});
