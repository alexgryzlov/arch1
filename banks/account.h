#ifndef BANKS_ACCOUNT_H
#define BANKS_ACCOUNT_H

#include <map>

class Account {
public:
    Account(double balance, double percent) : balance_(balance), percent_(percent), to_add_(0) {}

    virtual double ChangeBalance(double delta, time_t current_time) = 0;

    void ForceChangeBalance(double new_balance) {
        balance_ = new_balance;
    }

    virtual bool Has(double sum, time_t current_time) {
        return balance_ >= sum;
    }

    // C++ virtual constructor pattern
    virtual Account *__clone() = 0;

    virtual void AddPercent() {
        if (balance_ >= 0) {
            to_add_ += percent_ * balance_;
        }
    }

    virtual void CommitPercent() {
        balance_ += to_add_;
        to_add_ = 0;
    }

    size_t id() const {
        return id_;
    }

    double percent() const {
        return percent_;
    }

    double balance() const {
        return balance_;
    }

protected:
    size_t id_;
    double percent_; // daily percent
    double balance_;
    double to_add_;
};

class DebitAccount : public Account {
public:
    DebitAccount(double balance, double percent) : Account(balance, percent) {}

    double ChangeBalance(double delta, time_t) override {
        delta = std::max(balance_ + delta, 0.) - balance_;
        balance_ += delta;
        return delta;
    }

    virtual DebitAccount *__clone() override {
        return new DebitAccount(balance_, percent_);
    }
};

class DepositAccount : public Account {
public:
    DepositAccount(double balance, double percent, time_t deposit_deadline) : Account(balance, percent),
                                                                              deposit_deadline_(
                                                                                      deposit_deadline) {
        percent_ = initial_balance_to_percent_.lower_bound(initial_balance_)->second;
    }

    double ChangeBalance(double delta, time_t current_time) override {
        if (delta < 0 and current_time < deposit_deadline_) {
            delta = 0;
        }
        delta = std::max(balance_ + delta, 0.) - balance_;
        balance_ += delta;
        return delta;
    }

    bool Has(double sum, time_t current_time) override {
        return current_time >= deposit_deadline_ and balance_ >= sum;
    }

    virtual DepositAccount *__clone() override {
        return new DepositAccount(percent_, balance_, deposit_deadline_);
    }

private:
    double initial_balance_;
    time_t deposit_deadline_;
    static const std::map<size_t, double> initial_balance_to_percent_;
};

const std::map<size_t, double> DepositAccount::initial_balance_to_percent_ = {
        {50'000,   0.03},
        {100'000,  0.035},
        {SIZE_MAX, 0.04}
};

class CreditAccount : public Account {
public:
    CreditAccount(double balance, double commission) : Account(0., balance), commission_(commission) {}

    double ChangeBalance(double delta, time_t) override {
        if (balance_ < 0) {
            balance_ -= commission_;
        }
        balance_ += delta;
        return delta;
    }

    bool Has(double sum, time_t) override {
        return true;
    }

    virtual CreditAccount *__clone() override {
        return new CreditAccount(balance_, commission_);
    }

private:
    double commission_;
};

#endif //BANKS_ACCOUNT_H
