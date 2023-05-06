class Operator {
    constructor(call, qth, speed, farnsworth, wordsworth) {
        this.call = call;
        this.qth = qth;
        console.log(call);
        this.sounder = new Sounder();
        this.sounder.pitch = 600 + 100 - Math.round(200 * Math.random());
        this.sounder.volume = 1 - Math.random() / 2;
        this.setSpeed(speed, farnsworth, wordsworth);
        this.encoder = new Encoder(x => this.sounder.send(x));
        this.message = ' ';
        this.timeout = null;
        this.me = false;
        this.done = false;
    }

    get callSign() {
        return this.call;
    }

    get isDone() {
        return this.done;
    }

    setSpeed(wpm, farnsworth, wordsworth) {
        this.sounder.setSpeed(speed - Math.round(5 * Math.random()), farnsworth, wordsworth);
    }

    trace(message) {
        console.log('OP: ' + this.call + ' MSG: ' + message);
    }

    send(message) {
        console.log('REPLY: ' + message);
        setTimeout(() => { this.encoder.phrase(message); }, 300);
        this.message = ' ';
    }

    pick(list) {
        return list[Math.round((list.length - 1) * Math.random())];
    }

    pause() {
        this.trace('HEARD: ' + this.message);

        // again?
        var again = / AGN\? /g.exec(this.message);
        var question = / \? /g.exec(this.message);
        if (again || question) {
            this.trace('again');
            this.send(this.call);
            return;
        }

        // partial?
        var partial = /([^\ ]+)\?/g.exec(this.message);
        if (partial) {
            this.trace('partial: ' + partial);
            var call = partial[1];
            if (call == this.call) {
                this.trace('correct');
                this.me = true;
                this.send('RR ' + this.call + " <BK>");
                return;
            } else if (this.call.indexOf(call) >= 0) {
                this.trace('resend');
                this.send(this.call);
                return;
            }
        }

        // done?
        //var endingK = / K /g.exec(this.message);
        //var endingBK = / \<BK\> /g.exec(this.message);
        ///var done = endingK || endingBK;
        ///if (done) {
        //    this.trace('done');

            // calling CQ
            //var calling = /CQ DE ([^\ ]+).* K ?$/g.exec(_message);
            var calling = / CQ /g.exec(this.message);
            if (calling) {
                this.trace('calling');
                this.message = ' ';
                setTimeout(() => { this.send(this.call); /* + K or P2P */ }, 2000 * Math.random());
                return;
            }
        //}

        if (this.message.indexOf(' ' + this.call + ' ') >= 0) {
            this.me = true;
            var signal = this.pick(['5NN', '54N', '53N']);
            var ur = this.pick(['UR ', '']);
            this.send('<BK> TU ' + ur + signal + ' ' + signal + ' ' + this.qth + ' ' + this.qth + ' <BK>');
            return;
        }

        if (this.me) {
            var salutation = this.pick(['73', 'GL', '73 ES GL', ''])
            this.send(salutation + ' EE');
            this.done = true;
            return;
        }

        if (this.message.indexOf(' EE ') >= 0) {
            setTimeout(() => { this.send(this.call); /* + K or P2P */ }, 3000 + 2000 * Math.random());
        }

        this.message = ' ';
    }

    copy(character) {
        if (!this.done) {
            this.message += character.toUpperCase();
            if (this.timeout) clearTimeout(this.timeout);
            this.timeout = setTimeout(() => { this.pause(); }, 2000);
        }
    }
}