#coding=UTF-8
import os
import base64
from io import StringIO
from flask import Flask, render_template, redirect, url_for, flash, session, abort, make_response, jsonify, request
from werkzeug.security import generate_password_hash, check_password_hash
from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager, UserMixin, login_user, logout_user, current_user
from flask_bootstrap import Bootstrap
from flask.ext.wtf import Form
from wtforms import StringField, PasswordField, SubmitField
from wtforms.validators import Required, Length, EqualTo
import onetimepass
import datetime
import pyqrcode

# create application instance
app = Flask(__name__)
app.config.from_object('config')

# initialize extensions
bootstrap = Bootstrap(app)
db = SQLAlchemy(app)
lm = LoginManager(app)


class User(UserMixin, db.Model):
    """User model."""
    __tablename__ = 'users'
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(64), index=True)
    password_hash = db.Column(db.String(128))
    otp_secret = db.Column(db.String(16))

    def __init__(self, **kwargs):
        super(User, self).__init__(**kwargs)
        if self.otp_secret is None:
            # generate a random secret
            self.otp_secret = base64.b32encode(os.urandom(10)).decode('utf-8')

    @property
    def password(self):
        raise AttributeError('password is not a readable attribute')

    @password.setter
    def password(self, password):
        self.password_hash = generate_password_hash(password)

    def verify_password(self, password):
        return check_password_hash(self.password_hash, password)

    def get_totp_uri(self):
        return 'otpauth://totp/embeddeddemo:{0}?secret={1}&issuer=embeddeddemo'.format(self.username, self.otp_secret)

    def verify_totp(self, token):
        return onetimepass.valid_totp(token, self.otp_secret)

    def get_username(self):
        return self.username

    def change_password(self, password):
        self.password_hash = generate_password_hash(password)

class Log(db.Model):
    __tablename__ = 'log'
    id = db.Column(db.Integer, primary_key=True)
    type = db.Column(db.Integer)
    user = db.Column(db.String(128))
    log = db.Column(db.Text)
    time = db.Column(db.DateTime)
    def __init__(self, type, user, log):
        self.type = type
        self.user = user
        self.log = log
        self.time = datetime.datetime.now()

@lm.user_loader
def load_user(user_id):
    """User loader callback for Flask-Login."""
    return User.query.get(int(user_id))


class RegisterForm(Form):
    """Registration form."""
    username = StringField('Username', validators=[Required(), Length(1, 64)])
    password = PasswordField('Password', validators=[Required()])
    password_again = PasswordField('Password again', validators=[Required(), EqualTo('password')])
    submit = SubmitField('Register')


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/register', methods=['GET', 'POST'])
def register():
    """User registration route."""
    if current_user.is_authenticated:
        # if user is logged in we get out of here
        return redirect(url_for('index'))
    form = RegisterForm()
    if form.validate_on_submit():
        user = User.query.filter_by(username=form.username.data).first()
        if user is not None:
            flash('Username already exists.')
            return redirect(url_for('register'))
        # add new user to the database
        user = User(username=form.username.data, password=form.password.data)
        newLog = Log(1, "", 'Add user: ' + form.username.data)
        db.session.add(user)
        db.session.add(newLog)
        db.session.commit()

        # redirect to the two-factor auth page, passing username in session
        session['username'] = user.username
        return redirect(url_for('two_factor_setup'))
    return render_template('register.html', form=form)


@app.route('/twofactor')
def two_factor_setup():
    if 'username' not in session:
        return redirect(url_for('index'))
    user = User.query.filter_by(username=session['username']).first()
    if user is None:
        return redirect(url_for('index'))
    return render_template('two-factor-setup.html'), 200, {'Cache-Control': 'no-cache, no-store, must-revalidate', 'Pragma': 'no-cache', 'Expires': '0'}


@app.route('/qrcode')
def qrcode():
    if 'username' not in session:
        abort(404)
    user = User.query.filter_by(username=session['username']).first()
    if user is None:
        abort(404)

    del session['username']
    print user.get_username()
    url = pyqrcode.create(user.get_totp_uri())
    stream = StringIO()
    url.svg("templates/svg/"+user.get_username()+".svg", scale=3)

    svg = render_template("svg/"+user.get_username()+".svg", width=800, height=800)
    response = make_response(svg)
    response.content_type = 'image/svg+xml'

    return response


@app.route('/login', methods=['GET', 'POST'])
def login():
    user = User.query.filter_by(username=request.form['username']).first()
    print request.form['username']
    print user.verify_password(request.form['password'])
    print user.verify_totp(request.form['token'])

    if user is None or not user.verify_password(request.form['password']) or not user.verify_totp(request.form['token']):
        print "wrong"
        if user is None:
            newLog = Log(3, '', 'Someone failed to login')
        else:
            newLog = Log(3, request.form['username'], request.form['username'] +' failed to login')
        db.session.add(newLog)
        db.session.commit()
        return jsonify({"res": 0})

    newLog = Log(2, request.form['username'], request.form['username'] +' login')
    db.session.add(newLog)
    db.session.commit()
    print "right"
    return jsonify({"res": 1})

@app.route('/update', methods=['GET', 'POST'])
def update():
    user = User.query.filter_by(username=request.form['username']).first()
    if user is None or not user.verify_password(request.form['password']) or not user.verify_totp(request.form['token']):
        if user is None:
            newLog = Log(5, '', 'Someone failed to update password')
        else:
            newLog = Log(5, request.form['username'], request.form['username'] +' failed to update password')
        db.session.add(newLog)
        db.session.commit()
        return jsonify({"res":0})
    newLog = Log(4, request.form['username'], request.form['username'] +' updated password')
    db.session.add(newLog)
    db.session.commit()
    user.change_password(request.form['new_password'])
    db.session.commit()
    return jsonify({'res': 1})

@app.route("/log", methods=['GET', 'POST'])
def log():
    newLog = Log(1, 'a', 'test')
    db.session.add(newLog)
    db.session.commit()
    return jsonify({'res': 1})

@app.route("/show", methods=['GET'])
def show_log():
    data = Log.query.order_by(Log.time.desc()).all()
    for x in data:
        print x.log
    return render_template('log.html', data = data)
db.create_all()


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
